// #include "pch.h"

#include "Sensor.h"

#include <deque>
#include <vector>

namespace util::Sensor {
    namespace {
        std::deque<std::pair<std::int64_t, SensorData>> accelerators;
        std::deque<std::pair<std::int64_t, SensorData>> gyroscopes;
        std::deque<std::pair<int, std::int64_t>> frameTimestamps;
        std::size_t previousIndex = 0;
        std::size_t index = 0;
        std::size_t count = 0;
        // std::deque<Combination> data;
    } // namespace

    void init(int frameFreq, int sensorFreq) noexcept {
        // TODO set event listener and start sensor
    }

    std::vector<Combination> getData(int start, int stop) noexcept {
        // TODO get start timestamp and stop timestamp via frameTimestamps
        // use previousIndex as startIndex
        // travel the accelerators from startIndex to time over the stop timestamp
        // combine accelerator & gyroscope
        // remove 0-startIndex imu data
        // set previousIndex to imu data length
        return {};
    }

    void uninit() noexcept {
        // TODO stop sensor and clear event listener
    }
} // namespace util::Sensor

// #include <jni.h>
#include <android/log.h>
#include <android/sensor.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "SensorManager.h"

#define TAG "SensorManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__)

static volatile int sensorThreadFlag = 0;

pthread_t sensor_get_thread = 0;

static float* gQuaternion = NULL;
static float* gPreQuaternion = NULL;
static float* gUnityQuaternion = NULL;

pthread_mutex_t gmut;

void* Sensor_Dump_ACC_GYROSCOPE(void* param) {

    int err = 0;
    int accMinDelay = 0, gyroscopeDelay = 0;

#undef DUMP
#define DUMP 1
#if DUMP
    char* sensorRatePath = "/sdcard/Alva/sensor/rate.txt";
    FILE* fp = NULL;
    char rate_s[10] = {0};
    fp = fopen(sensorRatePath, "r+");
    fgets(rate_s, 10, fp);
    fclose(fp);
#endif

    ASensorManager* sensorManager = ASensorManager_getInstance();

    ASensor const* accSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    if (accSensor == NULL) {
        return NULL;
    }
    ASensor const* gyroSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_GYROSCOPE);
    if (gyroSensor == NULL) {
        return NULL;
    }

    ALooper* sensorLooper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ASensorEventQueue* sensorQueue = ASensorManager_createEventQueue(sensorManager, sensorLooper, 1, NULL, NULL);
    accMinDelay = ASensor_getMinDelay(accSensor);
    gyroscopeDelay = ASensor_getMinDelay(gyroSensor);

#if DUMP
    int rate_int = atoi(rate_s);
    LOGE("rate_string : %s      %d", rate_s, rate_int);
    accMinDelay = 1000000 / rate_int;
    gyroscopeDelay = 1000000 / rate_int;
#endif

    err = ASensorEventQueue_enableSensor(sensorQueue, accSensor);
    if (err < 0) {
        ASensorManager_destroyEventQueue(sensorManager, sensorQueue);
        return NULL;
    }
    err = ASensorEventQueue_enableSensor(sensorQueue, gyroSensor);
    if (err < 0) {
        ASensorManager_destroyEventQueue(sensorManager, sensorQueue);
        return NULL;
    }

    err = ASensorEventQueue_setEventRate(sensorQueue, accSensor, accMinDelay);
    if (err < 0) {
        ASensorEventQueue_disableSensor(sensorQueue, accSensor);
        ASensorManager_destroyEventQueue(sensorManager, sensorQueue);
        return NULL;
    }
    err = ASensorEventQueue_setEventRate(sensorQueue, gyroSensor, gyroscopeDelay);
    if (err < 0) {
        ASensorEventQueue_disableSensor(sensorQueue, gyroSensor);
        ASensorManager_destroyEventQueue(sensorManager, sensorQueue);
        return NULL;
    }
#if DUMP
    char acce_file[100] = {0};
    sprintf(acce_file, "/sdcard/Alva/dump/acce_%d.txt", rate_int);

    char gyro_file[100] = {0};
    sprintf(gyro_file, "/sdcard/Alva/dump/gyro_%d.txt", rate_int);

    FILE* fp_acc = NULL;
    fp_acc = fopen(acce_file, "w+");
    if (fp_acc == NULL) {
        LOGE("open fp_acc error.");
    }
    FILE* fp_gyro = NULL;
    fp_gyro = fopen(gyro_file, "w+");
    if (fp_gyro == NULL) {
        LOGE("open fp_gyro error.");
    }
#endif
    float rt[9] = {0.f};
    ASensorEvent event;
    while (sensorThreadFlag == 0) {
        if (ASensorEventQueue_getEvents(sensorQueue, &amp; event, 1) > 0) {
            switch (event.type) {
            case ASENSOR_TYPE_ACCELEROMETER: {
                float xAcc = event.vector.x;
                float yAcc = event.vector.y;
                float zAcc = event.vector.z;
                rt[0] = xAcc;
                rt[1] = yAcc;
                rt[2] = zAcc;

#if DUMP
                LOGE("%lld,%f,%f,%f\n", event.timestamp, xAcc, yAcc, zAcc);
                fprintf(fp_acc, "%lld,%f,%f,%f\n", event.timestamp, xAcc, yAcc, zAcc);
                fflush(fp_acc);
#endif
                setQuaternion(rt);
            }; break;
            case ASENSOR_TYPE_GYROSCOPE: {
                float xGyro = event.vector.x;
                float yGyro = event.vector.y;
                float zGyro = event.vector.z;
                rt[3] = xGyro;
                rt[4] = yGyro;
                rt[5] = zGyro;
#if DUMP
                LOGE("%lld,%f,%f,%f\n", event.timestamp, xGyro, yGyro, zGyro);
                fprintf(fp_gyro, "%lld,%f,%f,%f\n", event.timestamp, xGyro, yGyro, zGyro);
                fflush(fp_gyro);
#endif
                setQuaternion(rt);
            }; break;
            default:
                break;
            }
        }
    }

#if DUMP
    fclose(fp_acc);
    fclose(fp_gyro);
#endif

    pthread_exit(0);
    return NULL;
}

int initSensorManager() {
    sensorThreadFlag = 0;
    LOGE("start Create pthread");
    pthread_mutex_init(&amp; gmut, NULL);
    if (gQuaternion == NULL)
        gQuaternion = (float*)calloc(1, sizeof(float) * 9);
    if (gUnityQuaternion == NULL)
        gUnityQuaternion = (float*)calloc(1, sizeof(float) * 9);
    if (gPreQuaternion == NULL)
        gPreQuaternion = (float*)calloc(1, sizeof(float) * 9);
    gPreQuaternion[3] = 1.f;

    pthread_create(&amp; sensor_get_thread, NULL, Sensor_Dump_ACC_GYROSCOPE, (void*)NULL);
    LOGE("end Create pthread");
    return 0;
}

int unitSensorManager() {
    sensorThreadFlag = 1;
    pthread_join(sensor_get_thread, NULL);
    if (gQuaternion)
        free(gQuaternion);
    if (gUnityQuaternion)
        free(gUnityQuaternion);
    if (gPreQuaternion)
        free(gPreQuaternion);
    return 0;
}

#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

int initSensorManager();
void getQuaternion(float* unityQuaternion);
int unitSensorManager();

#ifdef __cplusplus
}
#endif

#endif

#define USE_ANDROID 1

#if USE_ANDROID == 1

#include <jni.h>

#endif

#include <android/log.h>
#include <stdio.h>

#include "SensorManager.h"
#include "com_example_testsensor_SensorInterface.h"

#define TAG "SensorInterface"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__)
extern "C" {
#if USE_ANDROID == 1
JNIEXPORT jint JNICALL Java_com_example_testsensor_SensorInterface_initManager(JNIEnv* env, jclass obj) {
#else
int initManager() {
#endif

    return initSensorManager();
}
#if USE_ANDROID == 1
JNIEXPORT jint JNICALL Java_com_example_testsensor_SensorInterface_unitManager(JNIEnv* env, jclass obj) {
#else
int unitManager() {
#endif

    return unitSensorManager();
}
#if USE_ANDROID == 1
JNIEXPORT jint JNICALL Java_com_example_testsensor_SensorInterface_getQuat(JNIEnv* env, jclass obj, jfloatArray ptr_Quat) {
    jfloat* arr = NULL;
    arr = env->GetFloatArrayElements(ptr_Quat, NULL);
    getQuaternion((float*)arr);

    env->ReleaseFloatArrayElements(ptr_Quat, arr, 0);

    return 0;
}
#else
int getQuat(float* ptr_Quat) {
    getQuaternion((float*)ptr_Quat);
    return 0;
}
#endif
}

/* DO NOT EDIT THIS FILE - it is machine generated */
#define USE_ANDROID 1
#if USE_ANDROID == 1

#include <jni.h>

#endif
/* Header for class com_example_testsensor_SensorInterface */

#ifndef _Included_com_example_testsensor_SensorInterface
#define _Included_com_example_testsensor_SensorInterface
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_example_testsensor_SensorInterface
 * Method:    setSensorArray
 * Signature: (II)I
 */
#if USE_ANDROID == 1
JNIEXPORT jint JNICALL Java_com_example_testsensor_SensorInterface_getQuat(JNIEnv* env, jclass obj, jfloatArray ptr_sensor);

JNIEXPORT jint JNICALL Java_com_example_testsensor_SensorInterface_initManager(JNIEnv* env, jclass obj);

JNIEXPORT jint JNICALL Java_com_example_testsensor_SensorInterface_unitManager(JNIEnv* env, jclass obj);
#else
int getQuat(float* ptr_sensor);
int initManager();
int unitManager();
#endif
#ifdef __cplusplus
}
#endif
#endif

package com.example.testsensor;

import android.util.Log;

public

class SensorInterface {
public

    static native int getQuat(float[] ptr_quat);

public

    static native int initManager();

public

    static native int unitManager();

    static {
        Log.e("load library", "start");
        System.loadLibrary("Sensor");
        Log.e("load library", "end");
    }
}

package

    com.example.testsensor;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;
import android.view.View;

public

class MainActivity extends

    Activity {

public
    float sensor_Quat[] = new float[9];
private
    Handler mHandler = new Handler();
private

    Runnable mRunnable = new Runnable() {

        @Override public void run() {
            // TODO Auto-generated method stub
            SensorInterface.getQuat(sensor_Quat);
            TextView textview0 = (TextView)findViewById(R.id.textView1);
            TextView textview1 = (TextView)findViewById(R.id.textView2);
            TextView textview2 = (TextView)findViewById(R.id.textView3);
            TextView textview3 = (TextView)findViewById(R.id.textView4);
            TextView textview4 = (TextView)findViewById(R.id.textView5);
            TextView textview5 = (TextView)findViewById(R.id.textView6);
            TextView textview6 = (TextView)findViewById(R.id.textView7);
            TextView textview7 = (TextView)findViewById(R.id.textView8);
            TextView textview8 = (TextView)findViewById(R.id.textView9);
            textview0.setText(Float.toString(sensor_Quat[0]));
            textview1.setText(Float.toString(sensor_Quat[1]));
            textview2.setText(Float.toString(sensor_Quat[2]));

            textview3.setText(Float.toString(sensor_Quat[3]));
            textview4.setText(Float.toString(sensor_Quat[4]));
            textview5.setText(Float.toString(sensor_Quat[5]));

            textview6.setText(Float.toString(sensor_Quat[6]));
            textview7.setText(Float.toString(sensor_Quat[7]));
            textview8.setText(Float.toString(sensor_Quat[8]));
            // textview3.setText(Float.toString(sensor_Quat[3]));
            mHandler.postDelayed(mRunnable, 10);
        }
    };
    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        SensorInterface.

            initManager();

        mHandler.post(mRunnable);
    }
}
