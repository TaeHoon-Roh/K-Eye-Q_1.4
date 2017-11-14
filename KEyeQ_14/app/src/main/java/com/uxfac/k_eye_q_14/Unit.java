package com.uxfac.k_eye_q_14;

import android.graphics.Bitmap;
import android.graphics.Rect;
import android.hardware.Camera;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.util.ArrayList;

/**
 * Created by kaist on 2017-06-13.
 */

public class Unit {
    public static byte[] result_buf_1d = new byte[9600];
    public static int CNN_xBuf_size = 0;
    public static float[] CNN_output_prev = new float[4800];
    public static float[] CNN_output = new float[512];      // 1x512 Vectors
    public static float[][] fc1_weight = new float[512][4800];
    public static float[] Person_Face = new float[512];

    public static float min_prob = 0;
    public static float min_prob2 = 0;
    public static float min_dist = 2;
    public static float min_dist2 = 2;
    public static int min_index = 0;
    public static int min_index2 = 0;
    public static float dist_thresh_init = (float) 0.3;

    public static ArrayList<Person> P_Data = new ArrayList<Person>();

    public static String Rank1, Rank2;



//이미지용
    public static boolean camera_frame_flag = true;
    public static boolean last_detect_face = false;

    public static Bitmap buff;

    public static Bitmap check;
    public static Bitmap pass_check;
    public static Bitmap Real_Face;
    public static Bitmap detectfaceBitmap;




//블루투스 용
    public static UsbManager usbManager;
    public static UsbDevice myDevice;
    public static UsbInterface usbInterface;
    public static UsbEndpoint usbEndpoint_1, usbEndpoint_2;
    public static UsbDeviceConnection connection;

    public static boolean face_detect_flag = false;


    //프로그래스 용
    public static boolean abow_c_flag = false;
    public static boolean init_file_flag = false;



    public static boolean s_flag = false;

    public static boolean ap_flag = false;

    public static boolean FaceDetectStartFlag = false;
    public static boolean AppExit = false;

    public static int Choice_Info = 0;
    public static boolean p_add_flag = false;
    public static int thread_sleep_time = 3000;


    public static int Camera_Toggle = Camera.CameraInfo.CAMERA_FACING_FRONT;

    public static Rect faceRect;

    public Unit(){

    }

    public static void init_Unit(){
        CNN_xBuf_size = 0;
        pass_check = null;
        CNN_output = new float[512];
        CNN_output_prev = new float[4800];
        result_buf_1d = new byte[9600];
        Person_Face = new float[512];

        dist_thresh_init = (float)0.3;
        min_prob = 0;
        min_prob2 = 0;
        min_dist = 2;
        min_dist2 = 2;
        min_index = 0;
        min_index2 = 0;
    }

    public static void InputCameraDataFlag(){
        Log.i("Unit", " : InputCameraDataFlag Init!!");
        face_detect_flag = true;
        last_detect_face = false;
        camera_frame_flag = true;
    }



}
