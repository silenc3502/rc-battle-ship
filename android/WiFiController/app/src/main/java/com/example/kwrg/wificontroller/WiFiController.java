package com.example.kwrg.wificontroller;

/**
 * Created by kwrg on 17. 5. 31.
 */

public class WiFiController {
    static{
        System.loadLibrary("native-lib");
    }
    public native String stringFromJNI();

    public native void WiFiConnect();

    public native int WiFiSendData(int no);
}
