package com.uxfac.k_eye_q_14;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;
import android.widget.Toast;

import java.io.DataOutputStream;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Iterator;

/**
 * Created by user on 2017-10-22.
 */

public class usbReceiver extends BroadcastReceiver {

    boolean one_time = false;
    boolean twe_time = false;
    boolean start_time = false;
    boolean set_time = false;

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();

        if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
            startAPP(context, intent);
            Toast.makeText(context,"Attached", Toast.LENGTH_SHORT).show();

        } else if (UsbManager.ACTION_USB_ACCESSORY_DETACHED.equals(action)) {

            UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            set_time = false;
            one_time = false;
            twe_time = false;
            start_time = false;
            Toast.makeText(context, "BoradCast", Toast.LENGTH_SHORT).show();
            if (device != null) {

            }
        } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {

            UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            set_time = false;
            Toast.makeText(context, "BoradCast", Toast.LENGTH_SHORT).show();
            if (device != null) {


            }
        }
    }

    public void startAPP(Context context, Intent intent) {
        UsbManager manager;
        HashMap<String, UsbDevice> deviceList;


        while (true) {
            manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
            deviceList = manager.getDeviceList();

            Log.i("startAPP", "deviceList : " + deviceList.size());
            if (deviceList.size() != 0 && set_time == false) {
                if (one_time == false) {
                    Log.i("Data_Thread", "one_time");
                    sudo_exe(context);
                } else if (twe_time == false) {
                    Log.i("Data_Thread", "twe_time");
                    sudo_exe(context);
                } else if (start_time == true) {
                    Log.i("Data_Thread", "start_time");
                    if (Unit.s_flag == true) {
                        ac_start(context);
                    } else {

                    }

                }
            } else {
                one_time = false;
                twe_time = false;
                start_time = false;
                Unit.s_flag = true;

                break;
            }

        }
    }

    private void sudo_exe(Context context) {
        Runtime runtime = Runtime.getRuntime();
        Process process;

        try {
            Log.d("test", "??");
            InputStream is = null;
            DataOutputStream os = null;

            Log.i("sudo_thread", "adb shell start");
            Log.i("sudo_thread", "adb shell end");
            Log.i("sudo_thread", "su start");
            process = runtime.exec("su");
            Log.i("sudo_thread", "fxload start");
            os = new DataOutputStream(process.getOutputStream());
            is = process.getInputStream();

            //300s
            //os.write(("./system/bin/fxload -i /system/lib/usb_stream_firmware.img -t fx3 -d 04b4:00f3" + "\n").getBytes("UTF-8"));

            //330s
            os.write(("./system/lib/fxload -i /system/lib/usb_stream_firmware.img -t fx3 -d 04b4:00f3" + "\n").getBytes("UTF-8"));

            Log.i("sudo_thread", "fxload end");
            os.writeBytes("exit\n");
            os.flush();
            int ret = process.waitFor();
            Log.i("sudo_thread", "su end");

            is.close();
            os.close();
        } catch (Exception e) {
            e.fillInStackTrace();
            Log.e("Process Manager", "Unable to execute top command");
        }

        Unit.usbManager = (UsbManager) context.getSystemService(context.USB_SERVICE);
        HashMap<String, UsbDevice> buff = new HashMap<String, UsbDevice>();
        buff = Unit.usbManager.getDeviceList();

        Iterator<UsbDevice> deviceIterator = buff.values().iterator();
        String i = "";
        int count = 0;
        while (deviceIterator.hasNext()) {
            System.out.println("usb Check : " + count++);
            UsbDevice device = deviceIterator.next();
            Unit.myDevice = device;
            i += "\n" +
                    "DeviceID: " + device.getDeviceId() + "\n" +
                    "DeviceName: " + device.getDeviceName() + "\n" +
                    "DeviceClass: " + device.getDeviceClass() + " - "
                    + "DeviceClass : " + device.getDeviceClass() + "\n" +
                    "DeviceSubClass: " + device.getDeviceSubclass() + "\n" +
                    "VendorID: " + device.getVendorId() + "\n" +
                    "ProductID: " + device.getProductId() + "\n";
        }

        if (one_time == false) {
            one_time = true;
        } else if (twe_time == false) {
            twe_time = true;
            start_time = true;
        } else {

        }

    }

    private void ac_start(Context context) {
        Unit.s_flag = false;

        getUsbManager();
        Intent intent = new Intent(context, InitDataActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
        set_time = true;
        start_time = false;
    }

    private void getUsbManager(){

        if(Unit.myDevice.getInterface(0) == null){

        }
        Unit.usbInterface = Unit.myDevice.getInterface(0);

        Unit.usbEndpoint_1 = Unit.usbInterface.getEndpoint(0);
        Unit.usbEndpoint_2 = Unit.usbInterface.getEndpoint(1);
        Unit.connection = Unit.usbManager.openDevice(Unit.myDevice);
        Log.i("Endpoint1", " : " +Unit.usbEndpoint_1);
        Log.i("Endpoint2", " : " +Unit.usbEndpoint_2);
        Log.i("connect", "Connection End!!");
    }

}
