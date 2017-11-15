#include <jni.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include "usb/libusb.h"
#include "ezusb-lib.h"

using namespace std;

static string path = "/data/data/com.uxfac.k_eye_q_14/files/test.txt";
static string print_devs(libusb_device **devs){
    libusb_device *dev;
    int i = 0, j = 0;
    int count = 0;
    uint8_t path[8];
    std::string temp;
    std::string result[10];
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;

        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            temp = "failed to get device descriptor";
            return temp;
        }

        printf("%04x:%04x (bus %d, device %d)",
               desc.idVendor, desc.idProduct,
               libusb_get_bus_number(dev), libusb_get_device_address(dev));

        char idvendor[21];
        char idproduct[21];
        char busnumber[21];
        char devaddress[21];
        char c[20];
        sprintf(idvendor,"%04x : ",desc.idVendor);
        sprintf(idproduct,"%04x\t",desc.idProduct);
        sprintf(busnumber,"bus : %d, ",libusb_get_bus_number(dev));
        sprintf(devaddress,"device : %d",libusb_get_device_address(dev));
        sprintf(c,"\tcount : %d\n",count);

        result[i] = idvendor;
        result[i] = result[i] + idproduct;
        result[i] = result[i] + busnumber;
        result[i] = result[i] + devaddress;
        result[i] = result[i] + c;
        count++;
    }
    for(int k = 0 ; k < count ; k++){
        temp = temp + result[k];
    }


    return temp;

}

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_uxfac_k_1eye_1q_114_InitDataActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    
    string hello = "Hello from C++";
    /*libusb_device **devs;
    int r;
    ssize_t cnt;

    r = libusb_init(NULL);
    if(r < 0){
        hello = "What the : "+r;
        return env->NewStringUTF(hello.c_str());
    }

    cnt = libusb_get_device_list(NULL, &devs);
    if(cnt < 0){
        char numstr[21];
        sprintf(numstr,"%d",cnt);
        hello = hello + numstr;
        return env->NewStringUTF(hello.c_str());
    }

    hello = print_devs(devs);
    libusb_free_device_list(devs,1);
    libusb_exit(NULL);
    int num = testOutput();
    char numstr[21];
    sprintf(numstr,"%d",num);
    hello = numstr;*/

    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_uxfac_k_1eye_1q_114_InitDataActivity_ezusbJNI(
        JNIEnv *env,
        jobject /* this */) {

    string hello = "hi";
    int num = testOutput();
    char numstr[21];
    sprintf(numstr,"%d",num);
    hello = numstr;
    char ex[1000] = "hihi";

    ifstream inFile(path);
    int size = inFile.tellg();
    if(!inFile.eof()){

        inFile.close();
    }
    sprintf(ex,"%d",size);
    hello = ex;


    return env->NewStringUTF(hello.c_str());
}

