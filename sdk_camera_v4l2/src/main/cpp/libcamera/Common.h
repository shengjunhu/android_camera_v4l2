//
// Created by Hsj on 2021/5/8.
//

#ifndef ANDROID_CAMERA_V4L2_COMMON_H
#define ANDROID_CAMERA_V4L2_COMMON_H

#include <jni.h>
#include <android/log.h>

//定义JNI日志
#ifdef LOG_SWITCH
    #define LOGI(TAG,...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__);
    #define LOGD(TAG,...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);
    #define LOGW(TAG,...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__);
    #define LOGE(TAG,...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);
    #define START(TAG) LOGD("start");
    #define END(TAG) LOGD("end");
#else
    #define LOGI(TAG,...) NULL;
    #define LOGD(TAG,...) NULL;
    #define LOGW(TAG,...) NULL;
    #define LOGE(TAG,...) NULL;
    #define START(TAG) NULL;
    #define END(TAG) NULL;
#endif

//编译器优化
//LIKELY    判断为真的可能性更大
//UNLIKELY  判断为假的可能性更大
//CONDITION 条件判断
#ifdef __GNUC__
    #define LIKELY(X) __builtin_expect(!!(X), 1)
    #define UNLIKELY(X) __builtin_expect(!!(X), 0)
    #define CONDITION(cond) __builtin_expect(cond!=0, 0)
#else
    #define LIKELY(X) (X)
    #define UNLIKELY(X) (X)
    #define	CONDITION(cond)	(cond)
#endif

#define SAFE_CLEAR(x) {memset (&(x), 0, sizeof (x));}
#define SAFE_FREE(x) {if(x != NULL){free(x); x = NULL;}}
#define SAFE_DELETE(p) {if (p) { delete (p); (p) = NULL;}}

typedef enum {
    PIXEL_FORMAT_RGB   = 0,
    PIXEL_FORMAT_RGBA  = 1,
    PIXEL_FORMAT_NV12  = 2,
    PIXEL_FORMAT_YUYV  = 3,
    PIXEL_FORMAT_GRAY  = 4,
    PIXEL_FORMAT_DEPTH = 5,
    PIXEL_FORMAT_ERROR = 6,
} PixelFormat;

void setVM(JavaVM *);
JavaVM *getVM();
JNIEnv *getEnv();
uint64_t timeMs();
uint64_t timeUs();

#endif //ANDROID_CAMERA_V4L2_COMMON_H
