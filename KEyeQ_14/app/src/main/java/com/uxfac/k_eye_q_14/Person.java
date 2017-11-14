package com.uxfac.k_eye_q_14;

import android.graphics.Bitmap;

/**
 * Created by kaist on 2017-06-13.
 */

public class Person {
    Bitmap face = null;
    String name = null;
    String info = null;
    double[] CNN_data = new double[512];
    Person(Bitmap face, String info, String name, double[] CNN_data){
        this.face = face;
        this.name = name;
        this.info = info;
        this.CNN_data = CNN_data;
    }
}
