package com.uxfac.k_eye_q_14;

import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

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
        if(view.getId() == R.id.button){
            tv1.setText(ezusbJNI());
        }
    }
}
