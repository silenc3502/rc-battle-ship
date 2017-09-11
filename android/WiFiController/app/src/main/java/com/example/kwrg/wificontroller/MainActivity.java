package com.example.kwrg.wificontroller;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Button;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    /*
    static {
        System.loadLibrary("native-lib");
    }
    */

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final WiFiController wifi = new WiFiController();
        wifi.WiFiConnect();

        Button button1 = (Button)findViewById(R.id.button);

        button1.setOnClickListener(new Button.OnClickListener(){
            public void onClick(View view){
                wifi.WiFiSendData(1);
            }
        });

        TextView tv = (TextView) findViewById(R.id.sample_text);
        //WiFiController wifi = new WiFiController();
        tv.setText(wifi.stringFromJNI());
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    //public native String stringFromJNI();
}
