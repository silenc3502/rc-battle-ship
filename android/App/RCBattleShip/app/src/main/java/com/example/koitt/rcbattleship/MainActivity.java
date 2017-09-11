package com.example.koitt.rcbattleship;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import static com.example.koitt.rcbattleship.R.layout;


public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    RelativeLayout layout_joystick;
    ImageView image_joystick, image_border;


    JoyStickClass js;


    @Override
    protected void onCreate(Bundle savezdInstanceState) {
        super.onCreate(savezdInstanceState);
        setContentView(layout.activity_main);

        layout_joystick = (RelativeLayout)findViewById(R.id.layout_joystick);

        js = new JoyStickClass(getApplicationContext(), layout_joystick, R.drawable.image_button);
        js.setStickSize(150, 150);
        js.setLayoutSize(500, 500);
        js.setLayoutAlpha(150);
        js.setStickAlpha(100);
        js.setOffset(90);
        js.setMinimumDistance(50);

        layout_joystick.setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View arg0, MotionEvent arg1) {
                js.drawStick(arg1);
                if(arg1.getAction() == MotionEvent.ACTION_DOWN
                        || arg1.getAction() == MotionEvent.ACTION_MOVE) {
                    int direction = js.get8Direction();
                    if(direction == JoyStickClass.STICK_UP) {

                    } else if(direction == JoyStickClass.STICK_UPRIGHT) {

                    } else if(direction == JoyStickClass.STICK_RIGHT) {

                    } else if(direction == JoyStickClass.STICK_DOWNRIGHT) {

                    } else if(direction == JoyStickClass.STICK_DOWN) {

                    } else if(direction == JoyStickClass.STICK_DOWNLEFT) {

                    } else if(direction == JoyStickClass.STICK_LEFT) {

                    } else if(direction == JoyStickClass.STICK_UPLEFT) {

                    } else if(direction == JoyStickClass.STICK_NONE) {

                    }
                } else if(arg1.getAction() == MotionEvent.ACTION_UP) {
                }
                return true;
            }
        });

        // Example of a call to a native method
        TextView tv = new TextView(this) ;
        tv.setText(stringFromJNI());

    }
    public native String stringFromJNI();



    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.btn1:
                Intent intent = new Intent(this, WIFIActivity.class);
                startActivity(intent);
                break;
            case R.id.btn2:
                Intent intent1 = new Intent(this, CannonActivity.class);
                startActivity(intent1);
                break;
            case R.id.btn3:
                Intent intent2 = new Intent(this, PowerActivity.class);
                startActivity(intent2);
                break;
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */

}
