//
// Created by root on 17. 11. 10.
//

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <jni.h>
#include <string>

#include "usb/libusb.h"
#include "ezusb-lib.h"

#if !defined(_WIN32) || defined(__CYGWIN__ )
#include <syslog.h>
static bool dosyslog = false;
#include <strings.h>
#endif

//fxload add
#include <stdarg.h>
#include <sys/types.h>
#include <getopt.h>
#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif


#if !defined(_WIN32) || defined(__CYGWIN__ )
#define _stricmp strcasecmp
#endif

#ifndef FXLOAD_VERSION
#define FXLOAD_VERSION (__DATE__ " (libusb)")
#endif

void logerror(const char *format, ...)
__attribute__ ((format (__printf__, 1, 2)));

void logerror(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

#if !defined(_WIN32) || defined(__CYGWIN__ )
    if (dosyslog)
        vsyslog(LOG_ERR, format, ap);
    else
#endif
        vfprintf(stderr, format, ap);
    va_end(ap);
}


int verbose = 1;

static bool fx_is_external(uint32_t addr, size_t len)
{
    if (addr <= 0x1b3f)
        return ((addr + len) > 0x1b40);

    return true;
}

static bool fx2_is_external(uint32_t addr, size_t len)
{
    if (addr <= 0x1fff)
        return ((addr + len) > 0x2000);

    else if (addr >= 0xe000 && addr <= 0xe1ff)
        return ((addr + len) > 0xe200);

    else
        return true;
}

static bool fx2lp_is_external(uint32_t addr, size_t len)
{
    if (addr <= 0x3fff)
        return ((addr + len) > 0x4000);

    else if (addr >= 0xe000 && addr <= 0xe1ff)
        return ((addr + len) > 0xe200);

    else
        return true;
}


#define RW_INTERNAL     0xA0	/* hardware implements this one */
#define RW_MEMORY       0xA3

static int ezusb_write(libusb_device_handle *device, const char *label,
                       uint8_t opcode, uint32_t addr, const unsigned char *data, size_t len)
{
    int status;

    if (verbose > 1)
        logerror("%s, addr 0x%08x len %4u (0x%04x)\n", label, addr, (unsigned)len, (unsigned)len);
    status = libusb_control_transfer(device,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     opcode, addr & 0xFFFF, addr >> 16,
                                     (unsigned char*)data, (uint16_t)len, 1000);
    if (status != (signed)len) {
        if (status < 0)
            logerror("%s: %s\n", label, libusb_error_name(status));
        else
            logerror("%s ==> %d\n", label, status);
    }
    return (status < 0) ? -EIO : 0;
}

static int ezusb_read(libusb_device_handle *device, const char *label,
                      uint8_t opcode, uint32_t addr, const unsigned char *data, size_t len)
{
    int status;

    if (verbose > 1)
        logerror("%s, addr 0x%08x len %4u (0x%04x)\n", label, addr, (unsigned)len, (unsigned)len);
    status = libusb_control_transfer(device,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     opcode, addr & 0xFFFF, addr >> 16,
                                     (unsigned char*)data, (uint16_t)len, 1000);
    if (status != (signed)len) {
        if (status < 0)
            logerror("%s: %s\n", label, libusb_error_name(status));
        else
            logerror("%s ==> %d\n", label, status);
    }
    return (status < 0) ? -EIO : 0;
}

static bool ezusb_cpucs(libusb_device_handle *device, uint32_t addr, bool doRun)
{
    int status;
    uint8_t data = doRun ? 0x00 : 0x01;

    if (verbose)
        logerror("%s\n", data ? "stop CPU" : "reset CPU");
    status = libusb_control_transfer(device,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     RW_INTERNAL, addr & 0xFFFF, addr >> 16,
                                     &data, 1, 1000);
    if ((status != 1) &&
        /* We may get an I/O error from libusb as the device disappears */
        ((!doRun) || (status != LIBUSB_ERROR_IO)))
    {
        const char *mesg = "can't modify CPUCS";
        if (status < 0)
            logerror("%s: %s\n", mesg, libusb_error_name(status));
        else
            logerror("%s\n", mesg);
        return false;
    } else
        return true;
}

static bool ezusb_fx3_jump(libusb_device_handle *device, uint32_t addr)
{
    int status;

    if (verbose)
        logerror("transfer execution to Program Entry at 0x%08x\n", addr);
    status = libusb_control_transfer(device,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     RW_INTERNAL, addr & 0xFFFF, addr >> 16,
                                     NULL, 0, 1000);
    /* We may get an I/O error from libusb as the device disappears */
    if ((status != 0) && (status != LIBUSB_ERROR_IO))
    {
        const char *mesg = "failed to send jump command";
        if (status < 0)
            logerror("%s: %s\n", mesg, libusb_error_name(status));
        else
            logerror("%s\n", mesg);
        return false;
    } else
        return true;
}

static int parse_ihex(FILE *image, void *context,
                      bool (*is_external)(uint32_t addr, size_t len),
                      int (*poke) (void *context, uint32_t addr, bool external,
                                   const unsigned char *data, size_t len))
{
    unsigned char data[1023];
    uint32_t data_addr = 0;
    size_t data_len = 0;
    int rc;
    int first_line = 1;
    bool external = false;

    for (;;) {
        char buf[512], *cp;
        char tmp, type;
        size_t len;
        unsigned idx, off;

        cp = fgets(buf, sizeof(buf), image);
        if (cp == NULL) {
            logerror("EOF without EOF record!\n");
            break;
        }

        if (buf[0] == '#')
            continue;

        if (buf[0] != ':') {
            logerror("not an ihex record: %s", buf);
            return -2;
        }

        cp = strchr(buf, '\n');
        if (cp)
            *cp = 0;

        if (verbose >= 3)
            logerror("** LINE: %s\n", buf);

        tmp = buf[3];
        buf[3] = 0;
        len = strtoul(buf+1, NULL, 16);
        buf[3] = tmp;

        tmp = buf[7];
        buf[7] = 0;
        off = (int)strtoul(buf+3, NULL, 16);
        buf[7] = tmp;

        if (first_line) {
            data_addr = off;
            first_line = 0;
        }

        tmp = buf[9];
        buf[9] = 0;
        type = (char)strtoul(buf+7, NULL, 16);
        buf[9] = tmp;

        if (type == 1) {
            if (verbose >= 2)
                logerror("EOF on hexfile\n");
            break;
        }

        if (type != 0) {
            logerror("unsupported record type: %u\n", type);
            return -3;
        }

        if ((len * 2) + 11 > strlen(buf)) {
            logerror("record too short?\n");
            return -4;
        }

        if (data_len != 0
            && (off != (data_addr + data_len)
                /* || !merge */
                || (data_len + len) > sizeof(data))) {
            if (is_external)
                external = is_external(data_addr, data_len);
            rc = poke(context, data_addr, external, data, data_len);
            if (rc < 0)
                return -1;
            data_addr = off;
            data_len = 0;
        }

        for (idx = 0, cp = buf+9 ;  idx < len ;  idx += 1, cp += 2) {
            tmp = cp[2];
            cp[2] = 0;
            data[data_len + idx] = (uint8_t)strtoul(cp, NULL, 16);
            cp[2] = tmp;
        }
        data_len += len;
    }


    if (data_len != 0) {
        if (is_external)
            external = is_external(data_addr, data_len);
        rc = poke(context, data_addr, external, data, data_len);
        if (rc < 0)
            return -1;
    }
    return 0;
}

static int parse_bin(FILE *image, void *context,
                     bool (*is_external)(uint32_t addr, size_t len), int (*poke)(void *context,
                                                                                 uint32_t addr, bool external, const unsigned char *data, size_t len))
{
    unsigned char data[4096];
    uint32_t data_addr = 0;
    size_t data_len = 0;
    int rc;
    bool external = false;

    for (;;) {
        data_len = fread(data, 1, 4096, image);
        if (data_len == 0)
            break;
        if (is_external)
            external = is_external(data_addr, data_len);
        rc = poke(context, data_addr, external, data, data_len);
        if (rc < 0)
            return -1;
        data_addr += (uint32_t)data_len;
    }
    return feof(image)?0:-1;
}

static int parse_iic(FILE *image, void *context,
                     bool (*is_external)(uint32_t addr, size_t len),
                     int (*poke)(void *context, uint32_t addr, bool external, const unsigned char *data, size_t len))
{
    unsigned char data[4096];
    uint32_t data_addr = 0;
    size_t data_len = 0, read_len;
    uint8_t block_header[4];
    int rc;
    bool external = false;
    long file_size, initial_pos;

    initial_pos = ftell(image);
    if (initial_pos < 0)
        return -1;

    if (fseek(image, 0L, SEEK_END) != 0)
        return -1;
    file_size = ftell(image);
    if (fseek(image, initial_pos, SEEK_SET) != 0)
        return -1;
    for (;;) {
        if (ftell(image) >= (file_size - 5))
            break;
        if (fread(&block_header, 1, sizeof(block_header), image) != 4) {
            logerror("unable to read IIC block header\n");
            return -1;
        }
        data_len = (block_header[0] << 8) + block_header[1];
        data_addr = (block_header[2] << 8) + block_header[3];
        if (data_len > sizeof(data)) {
            logerror("IIC data block too small - please report this error to libusb.info\n");
            return -1;
        }
        read_len = fread(data, 1, data_len, image);
        if (read_len != data_len) {
            logerror("read error\n");
            return -1;
        }
        if (is_external)
            external = is_external(data_addr, data_len);
        rc = poke(context, data_addr, external, data, data_len);
        if (rc < 0)
            return -1;
    }
    return 0;
}

static int (*parse[IMG_TYPE_MAX])(FILE *image, void *context, bool (*is_external)(uint32_t addr, size_t len),
                                  int (*poke)(void *context, uint32_t addr, bool external, const unsigned char *data, size_t len))
= { parse_ihex, parse_iic, parse_bin };

typedef enum {
    _undef = 0,
    internal_only,		/* hardware first-stage loader */
    skip_internal,		/* first phase, second-stage loader */
    skip_external		/* second phase, second-stage loader */
} ram_mode;

struct ram_poke_context {
    libusb_device_handle *device;
    ram_mode mode;
    size_t total, count;
};

#define RETRY_LIMIT 5

static int ram_poke(void *context, uint32_t addr, bool external,
                    const unsigned char *data, size_t len)
{
    struct ram_poke_context *ctx = (struct ram_poke_context*)context;
    int rc;
    unsigned retry = 0;

    switch (ctx->mode) {
        case internal_only:		/* CPU should be stopped */
            if (external) {
                logerror("can't write %u bytes external memory at 0x%08x\n",
                         (unsigned)len, addr);
                return -EINVAL;
            }
            break;
        case skip_internal:		/* CPU must be running */
            if (!external) {
                if (verbose >= 2) {
                    logerror("SKIP on-chip RAM, %u bytes at 0x%08x\n",
                             (unsigned)len, addr);
                }
                return 0;
            }
            break;
        case skip_external:		/* CPU should be stopped */
            if (external) {
                if (verbose >= 2) {
                    logerror("SKIP external RAM, %u bytes at 0x%08x\n",
                             (unsigned)len, addr);
                }
                return 0;
            }
            break;
        case _undef:
        default:
            logerror("bug\n");
            return -EDOM;
    }

    ctx->total += len;
    ctx->count++;

    while ((rc = ezusb_write(ctx->device,
                             external ? "write external" : "write on-chip",
                             external ? RW_MEMORY : RW_INTERNAL,
                             addr, data, len)) < 0
           && retry < RETRY_LIMIT) {
        if (rc != LIBUSB_ERROR_TIMEOUT)
            break;
        retry += 1;
    }
    return rc;
}

static int fx3_load_ram(libusb_device_handle *device, const char *path)
{
    uint32_t dCheckSum, dExpectedCheckSum, dAddress, i, dLen, dLength;
    uint32_t* dImageBuf;
    unsigned char *bBuf, hBuf[4], blBuf[4], rBuf[4096];
    FILE *image;
    int ret = 0;

    image = fopen(path, "rb");
    if (image == NULL) {
        logerror("unable to open '%s' for input\n", path);
        return -2;
    } else if (verbose)
        logerror("open firmware image %s for RAM upload\n", path);

    // Read header
    if (fread(hBuf, sizeof(char), sizeof(hBuf), image) != sizeof(hBuf)) {
        logerror("could not read image header");
        ret = -3;
        goto exit;
    }

    // check "CY" signature byte and format
    if ((hBuf[0] != 'C') || (hBuf[1] != 'Y')) {
        logerror("image doesn't have a CYpress signature\n");
        ret = -3;
        goto exit;
    }

    // Check bImageType
    switch(hBuf[3]) {
        case 0xB0:
            if (verbose)
                logerror("normal FW binary %s image with checksum\n", (hBuf[2]&0x01)?"data":"executable");
            break;
        case 0xB1:
            logerror("security binary image is not currently supported\n");
            ret = -3;
            goto exit;
        case 0xB2:
            logerror("VID:PID image is not currently supported\n");
            ret = -3;
            goto exit;
        default:
            logerror("invalid image type 0x%02X\n", hBuf[3]);
            ret = -3;
            goto exit;
    }

    // Read the bootloader version
    if (verbose) {
        if ((ezusb_read(device, "read bootloader version", RW_INTERNAL, 0xFFFF0020, blBuf, 4) < 0)) {
            logerror("Could not read bootloader version\n");
            ret = -8;
            goto exit;
        }
        logerror("FX3 bootloader version: 0x%02X%02X%02X%02X\n", blBuf[3], blBuf[2], blBuf[1], blBuf[0]);
    }

    dCheckSum = 0;
    if (verbose)
        logerror("writing image...\n");
    while (1) {
        if ((fread(&dLength, sizeof(uint32_t), 1, image) != 1) ||  // read dLength
            (fread(&dAddress, sizeof(uint32_t), 1, image) != 1)) { // read dAddress
            logerror("could not read image");
            ret = -3;
            goto exit;
        }
        if (dLength == 0)
            break; // done

        // coverity[tainted_data]
        dImageBuf = (uint32_t*)calloc(dLength, sizeof(uint32_t));
        if (dImageBuf == NULL) {
            logerror("could not allocate buffer for image chunk\n");
            ret = -4;
            goto exit;
        }

        // read sections
        if (fread(dImageBuf, sizeof(uint32_t), dLength, image) != dLength) {
            logerror("could not read image");
            free(dImageBuf);
            ret = -3;
            goto exit;
        }
        for (i = 0; i < dLength; i++)
            dCheckSum += dImageBuf[i];
        dLength <<= 2; // convert to Byte length
        bBuf = (unsigned char*) dImageBuf;

        while (dLength > 0) {
            dLen = 4096; // 4K max
            if (dLen > dLength)
                dLen = dLength;
            if ((ezusb_write(device, "write firmware", RW_INTERNAL, dAddress, bBuf, dLen) < 0) ||
                (ezusb_read(device, "read firmware", RW_INTERNAL, dAddress, rBuf, dLen) < 0)) {
                logerror("R/W error\n");
                free(dImageBuf);
                ret = -5;
                goto exit;
            }
            // Verify data: rBuf with bBuf
            for (i = 0; i < dLen; i++) {
                if (rBuf[i] != bBuf[i]) {
                    logerror("verify error");
                    free(dImageBuf);
                    ret = -6;
                    goto exit;
                }
            }

            dLength -= dLen;
            bBuf += dLen;
            dAddress += dLen;
        }
        free(dImageBuf);
    }

    // read pre-computed checksum data
    if ((fread(&dExpectedCheckSum, sizeof(uint32_t), 1, image) != 1) ||
        (dCheckSum != dExpectedCheckSum)) {
        logerror("checksum error\n");
        ret = -7;
        goto exit;
    }

    // transfer execution to Program Entry
    if (!ezusb_fx3_jump(device, dAddress)) {
        ret = -6;
    }

    exit:
    fclose(image);
    return ret;
}

int ezusb_load_ram(libusb_device_handle *device, const char *path, int fx_type, int img_type, int stage)
{
    FILE *image;
    uint32_t cpucs_addr;
    bool (*is_external)(uint32_t off, size_t len);
    struct ram_poke_context ctx;
    int status;
    uint8_t iic_header[8] = { 0 };
    int ret = 0;

    if (fx_type == FX_TYPE_FX3)
        return fx3_load_ram(device, path);

    image = fopen(path, "rb");
    if (image == NULL) {
        logerror("%s: unable to open for input.\n", path);
        return -2;
    } else if (verbose > 1)
        logerror("open firmware image %s for RAM upload\n", path);

    if (img_type == IMG_TYPE_IIC) {
        if ( (fread(iic_header, 1, sizeof(iic_header), image) != sizeof(iic_header))
             || (((fx_type == FX_TYPE_FX2LP) || (fx_type == FX_TYPE_FX2)) && (iic_header[0] != 0xC2))
             || ((fx_type == FX_TYPE_AN21) && (iic_header[0] != 0xB2))
             || ((fx_type == FX_TYPE_FX1) && (iic_header[0] != 0xB6)) ) {
            logerror("IIC image does not contain executable code - cannot load to RAM.\n");
            ret = -1;
            goto exit;
        }
    }

    /* EZ-USB original/FX and FX2 devices differ, apart from the 8051 core */
    switch(fx_type) {
        case FX_TYPE_FX2LP:
            cpucs_addr = 0xe600;
            is_external = fx2lp_is_external;
            break;
        case FX_TYPE_FX2:
            cpucs_addr = 0xe600;
            is_external = fx2_is_external;
            break;
        default:
            cpucs_addr = 0x7f92;
            is_external = fx_is_external;
            break;
    }

    /* use only first stage loader? */
    if (stage == 0) {
        ctx.mode = internal_only;

        /* if required, halt the CPU while we overwrite its code/data */
        if (cpucs_addr && !ezusb_cpucs(device, cpucs_addr, false))
        {
            ret = -1;
            goto exit;
        }

        /* 2nd stage, first part? loader was already uploaded */
    } else {
        ctx.mode = skip_internal;

        /* let CPU run; overwrite the 2nd stage loader later */
        if (verbose)
            logerror("2nd stage: write external memory\n");
    }

    /* scan the image, first (maybe only) time */
    ctx.device = device;
    ctx.total = ctx.count = 0;
    status = parse[img_type](image, &ctx, is_external, ram_poke);
    if (status < 0) {
        logerror("unable to upload %s\n", path);
        ret = status;
        goto exit;
    }

    /* second part of 2nd stage: rescan */
    // TODO: what should we do for non HEX images there?
    if (stage) {
        ctx.mode = skip_external;

        /* if needed, halt the CPU while we overwrite the 1st stage loader */
        if (cpucs_addr && !ezusb_cpucs(device, cpucs_addr, false))
        {
            ret = -1;
            goto exit;
        }

        /* at least write the interrupt vectors (at 0x0000) for reset! */
        rewind(image);
        if (verbose)
            logerror("2nd stage: write on-chip memory\n");
        status = parse_ihex(image, &ctx, is_external, ram_poke);
        if (status < 0) {
            logerror("unable to completely upload %s\n", path);
            ret = status;
            goto exit;
        }
    }

    if (verbose && (ctx.count != 0)) {
        logerror("... WROTE: %d bytes, %d segments, avg %d\n",
                 (int)ctx.total, (int)ctx.count, (int)(ctx.total/ctx.count));
    }

    /* if required, reset the CPU so it runs what we just uploaded */
    if (cpucs_addr && !ezusb_cpucs(device, cpucs_addr, true))
        ret = -1;

    exit:
    fclose(image);
    return ret;
}





//fxload

#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif


#define FIRMWARE 0
#define LOADER 1

static int print_usage(int error_code) {
    fprintf(stderr, "\nUsage: fxload [-v] [-V] [-t type] [-d vid:pid] [-p bus,addr] [-s loader] -i firmware\n");
    fprintf(stderr, "  -i <path>       -- Firmware to upload\n");
    fprintf(stderr, "  -s <path>       -- Second stage loader\n");
    fprintf(stderr, "  -t <type>       -- Target type: an21, fx, fx2, fx2lp, fx3\n");
    fprintf(stderr, "  -d <vid:pid>    -- Target device, as an USB VID:PID\n");
    fprintf(stderr, "  -p <bus,addr>   -- Target device, as a libusb bus number and device address path\n");
    fprintf(stderr, "  -v              -- Increase verbosity\n");
    fprintf(stderr, "  -q              -- Decrease verbosity (silent mode)\n");
    fprintf(stderr, "  -V              -- Print program version\n");
    return error_code;
}

int fxload(int argc, char*argv[]){
    fx_known_device known_device[] = FX_KNOWN_DEVICES;
    const char *path[] = { NULL, NULL };
    const char *device_id = NULL;
    const char *device_path = getenv("DEVICE");
    const char *type = NULL;
    const char *fx_name[FX_TYPE_MAX] = FX_TYPE_NAMES;
    const char *ext, *img_name[] = IMG_TYPE_NAMES;
    int fx_type = FX_TYPE_UNDEFINED, img_type[ARRAYSIZE(path)];
    int opt, status;
    unsigned int i, j;
    unsigned vid = 0, pid = 0;
    unsigned busnum = 0, devaddr = 0, _busnum, _devaddr;
    libusb_device *dev, **devs;
    libusb_device_handle *device = NULL;
    struct libusb_device_descriptor desc;

    while ((opt = getopt(argc, argv, "qvV?hd:p:i:I:s:S:t:")) != EOF)
        switch (opt) {

            case 'd':
                device_id = optarg;
                if (sscanf(device_id, "%x:%x" , &vid, &pid) != 2 ) {
                    fputs ("please specify VID & PID as \"vid:pid\" in hexadecimal format\n", stderr);
                    return -1;
                }
                break;

            case 'p':
                device_path = optarg;
                if (sscanf(device_path, "%u,%u", &busnum, &devaddr) != 2 ) {
                    fputs ("please specify bus number & device number as \"bus,dev\" in decimal format\n", stderr);
                    return -1;
                }
                break;

            case 'i':
            case 'I':
                path[FIRMWARE] = optarg;
                break;

            case 's':
            case 'S':
                path[LOADER] = optarg;
                break;

            case 'V':
                puts(FXLOAD_VERSION);
                return 0;

            case 't':
                type = optarg;
                break;

            case 'v':
                verbose++;
                break;

            case 'q':
                verbose--;
                break;

            case '?':
            case 'h':
            default:
                return print_usage(-1);

        }

    if (path[FIRMWARE] == NULL) {
        logerror("no firmware specified!\n");
        return print_usage(-1);
    }
    if ((device_id != NULL) && (device_path != NULL)) {
        logerror("only one of -d or -p can be specified\n");
        return print_usage(-1);
    }

    /* determine the target type */
    if (type != NULL) {
        for (i=0; i<FX_TYPE_MAX; i++) {
            if (strcmp(type, fx_name[i]) == 0) {
                fx_type = i;
                break;
            }
        }
        if (i >= FX_TYPE_MAX) {
            logerror("illegal microcontroller type: %s\n", type);
            return print_usage(-1);
        }
    }

    /* open the device using libusb */
    status = libusb_init(NULL);
    if (status < 0) {
        logerror("libusb_init() failed: %s\n", libusb_error_name(status));
        return -1;
    }
    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, verbose);

    /* try to pick up missing parameters from known devices */
    if ((type == NULL) || (device_id == NULL) || (device_path != NULL)) {
        if (libusb_get_device_list(NULL, &devs) < 0) {
            logerror("libusb_get_device_list() failed: %s\n", libusb_error_name(status));
            goto err;
        }
        for (i=0; (dev=devs[i]) != NULL; i++) {
            _busnum = libusb_get_bus_number(dev);
            _devaddr = libusb_get_device_address(dev);
            if ((type != NULL) && (device_path != NULL)) {
                // if both a type and bus,addr were specified, we just need to find our match
                if ((libusb_get_bus_number(dev) == busnum) && (libusb_get_device_address(dev) == devaddr))
                    break;
            } else {
                status = libusb_get_device_descriptor(dev, &desc);
                if (status >= 0) {
                    if (verbose >= 3) {
                        logerror("examining %04x:%04x (%d,%d)\n",
                                 desc.idVendor, desc.idProduct, _busnum, _devaddr);
                    }
                    for (j=0; j<ARRAYSIZE(known_device); j++) {
                        if ((desc.idVendor == known_device[j].vid)
                            && (desc.idProduct == known_device[j].pid)) {
                            if (// nothing was specified
                                    ((type == NULL) && (device_id == NULL) && (device_path == NULL)) ||
                                    // vid:pid was specified and we have a match
                                    ((type == NULL) && (device_id != NULL) && (vid == desc.idVendor) && (pid == desc.idProduct)) ||
                                    // bus,addr was specified and we have a match
                                    ((type == NULL) && (device_path != NULL) && (busnum == _busnum) && (devaddr == _devaddr)) ||
                                    // type was specified and we have a match
                                    ((type != NULL) && (device_id == NULL) && (device_path == NULL) && (fx_type == known_device[j].type)) ) {
                                fx_type = known_device[j].type;
                                vid = desc.idVendor;
                                pid = desc.idProduct;
                                busnum = _busnum;
                                devaddr = _devaddr;
                                break;
                            }
                        }
                    }
                    if (j < ARRAYSIZE(known_device)) {
                        if (verbose)
                            logerror("found device '%s' [%04x:%04x] (%d,%d)\n",
                                     known_device[j].designation, vid, pid, busnum, devaddr);
                        break;
                    }
                }
            }
        }
        if (dev == NULL) {
            libusb_free_device_list(devs, 1);
            libusb_exit(NULL);
            logerror("could not find a known device - please specify type and/or vid:pid and/or bus,dev\n");
            return print_usage(-1);
        }
        status = libusb_open(dev, &device);
        libusb_free_device_list(devs, 1);
        if (status < 0) {
            logerror("libusb_open() failed: %s\n", libusb_error_name(status));
            goto err;
        }
    } else if (device_id != NULL) {
        device = libusb_open_device_with_vid_pid(NULL, (uint16_t)vid, (uint16_t)pid);
        if (device == NULL) {
            logerror("libusb_open() failed\n");
            goto err;
        }
    }

    /* We need to claim the first interface */
    libusb_set_auto_detach_kernel_driver(device, 1);
    status = libusb_claim_interface(device, 0);
    if (status != LIBUSB_SUCCESS) {
        libusb_close(device);
        logerror("libusb_claim_interface failed: %s\n", libusb_error_name(status));
        goto err;
    }

    if (verbose)
        logerror("microcontroller type: %s\n", fx_name[fx_type]);

    for (i=0; i<ARRAYSIZE(path); i++) {
        if (path[i] != NULL) {
            ext = path[i] + strlen(path[i]) - 4;
            if ((_stricmp(ext, ".hex") == 0) || (strcmp(ext, ".ihx") == 0))
                img_type[i] = IMG_TYPE_HEX;
            else if (_stricmp(ext, ".iic") == 0)
                img_type[i] = IMG_TYPE_IIC;
            else if (_stricmp(ext, ".bix") == 0)
                img_type[i] = IMG_TYPE_BIX;
            else if (_stricmp(ext, ".img") == 0)
                img_type[i] = IMG_TYPE_IMG;
            else {
                logerror("%s is not a recognized image type\n", path[i]);
                goto err;
            }
        }
        if (verbose && path[i] != NULL)
            logerror("%s: type %s\n", path[i], img_name[img_type[i]]);
    }

    if (path[LOADER] == NULL) {
        /* single stage, put into internal memory */
        if (verbose > 1)
            logerror("single stage: load on-chip memory\n");
        status = ezusb_load_ram(device, path[FIRMWARE], fx_type, img_type[FIRMWARE], 0);
    } else {
        /* two-stage, put loader into internal memory */
        if (verbose > 1)
            logerror("1st stage: load 2nd stage loader\n");
        status = ezusb_load_ram(device, path[LOADER], fx_type, img_type[LOADER], 0);
        if (status == 0) {
            /* two-stage, put firmware into internal memory */
            if (verbose > 1)
                logerror("2nd state: load on-chip memory\n");
            status = ezusb_load_ram(device, path[FIRMWARE], fx_type, img_type[FIRMWARE], 1);
        }
    }

    libusb_release_interface(device, 0);
    libusb_close(device);
    libusb_exit(NULL);
    return status;
    err:
    libusb_exit(NULL);
    return -1;
}

int testOutput(){
    return -111;
}