/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_example_kwrg_wificontroller_WiFiController */

#ifndef _Included_com_example_kwrg_wificontroller_WiFiController
#define _Included_com_example_kwrg_wificontroller_WiFiController
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_example_kwrg_wificontroller_WiFiController
 * Method:    stringFromJNI
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_example_kwrg_wificontroller_WiFiController_stringFromJNI
  (JNIEnv *, jobject);

/*
 * Class:     com_example_kwrg_wificontroller_WiFiController
 * Method:    WiFiConnect
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_kwrg_wificontroller_WiFiController_WiFiConnect
  (JNIEnv *, jobject);

/*
 * Class:     com_example_kwrg_wificontroller_WiFiController
 * Method:    WiFiSendData
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_example_kwrg_wificontroller_WiFiController_WiFiSendData
  (JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif
#endif