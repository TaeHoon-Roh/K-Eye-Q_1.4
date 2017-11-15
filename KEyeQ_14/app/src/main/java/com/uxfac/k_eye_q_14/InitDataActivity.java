package com.uxfac.k_eye_q_14;

import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

public class InitDataActivity extends AppCompatActivity implements View.OnClickListener {
    TextView tv1;
    Button bt1;

    public native String stringFromJNI();
    public native String ezusbJNI();
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_init_data);

        int v = Build.VERSION.SDK_INT;
        Log.i("Version : ", ""+v);
        tv1 = (TextView)findViewById(R.id.textView);
        tv1.setText(stringFromJNI());
        bt1 = (Button)findViewById(R.id.button);
        bt1.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        String sdcardPath = null;
        String sdcardStat = Environment.getExternalStorageState();
        if(sdcardStat.equals(Environment.MEDIA_MOUNTED))
        {
            sdcardPath = Environment.getExternalStorageDirectory().getAbsolutePath();
        }

        byte[] temp;
        temp = new byte[100];
        try {
            FileInputStream fi = new FileInputStream(new File("/storage/emulated/0","test.txt"));
            fi.read(temp);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }


        if(view.getId() == R.id.button){
            tv1.setText(new String(temp));
        }
    }
}
