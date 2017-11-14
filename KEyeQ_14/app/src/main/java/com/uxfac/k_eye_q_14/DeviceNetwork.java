package com.uxfac.k_eye_q_14;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;

/**
 * Created by user on 2017-10-23.
 */

public class DeviceNetwork {
    Bitmap bitmap;
    byte[] buff;
    byte[] in_buff_1;
    byte[] in_buff_2;
    byte[] in_buff_3;
    Context context;
    Resources resources;

    public DeviceNetwork(Context context, Resources resources){
        this.context = context;
        this.resources = resources;
        this.buff = new byte[128*128];
        this.in_buff_1 = new byte[2];
        this.in_buff_2 = new byte[19200];
        this.in_buff_3 = new byte[2];
    }

    public void SendImage(Bitmap bitmap){
        byte[] buffer = ImageDataSerial(bitmap);
        Unit.connection.bulkTransfer(Unit.usbEndpoint_1, buffer, 128 * 128, 0);
        ReceiveCnnData();
    }

    public byte[] ReceiveCnnData(){
        byte[] result;
        result = new byte[19200];

        Unit.connection.bulkTransfer(Unit.usbEndpoint_2, in_buff_1, 2, 0);
        System.out.println(in_buff_1[0] + "\t" + in_buff_1[1]);
        Unit.connection.bulkTransfer(Unit.usbEndpoint_2, in_buff_2, 19200, 10000);
        Unit.connection.bulkTransfer(Unit.usbEndpoint_2, in_buff_3, 2, 0);
        System.out.println(in_buff_3[0] + "\t" + in_buff_3[1]);
        return result;
    }

    public byte[] ImageDataSerial(Bitmap bitmap){
        byte[] buffer_copy = new byte[128*128];
        for(int i = 0 ; i < 128 ; i++){
            for(int j = 0 ; j < 128 ; j++){
                buffer_copy[(i*128) + j] = (byte)bitmap.getPixel(i,j);
            }
        }
        return buffer_copy;
    }


}
