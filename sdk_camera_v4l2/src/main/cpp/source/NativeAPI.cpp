//
// Created by Hsj on 2021/4/26.
//

#include "Common.h"
#include "Camera.h"
#include "NativeAPI.h"

#define TAG "JNI"
#define OBJECT_ID "nativeObj"
#define CLASS_NAME "com/hsj/camera/CameraAPI"
typedef jlong CAMERA_ID;

static void setFieldLong(JNIEnv *env, jobject obj, const char *fieldName, jlong value) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID field = env->GetFieldID(clazz, fieldName, "J");
    if (LIKELY(field)) {
        env->SetLongField(obj, field, value);
    } else {
        LOGE(TAG, "setFieldLong: failed '%s' not found", fieldName)
    }
    env->DeleteLocalRef(clazz);
}

static CAMERA_ID nativeInit(JNIEnv *env, jobject thiz) {
    auto *camera = new Camera();
    auto cameraId = reinterpret_cast<CAMERA_ID>(camera);
    setFieldLong(env,thiz, OBJECT_ID, cameraId);
    return cameraId;
}

static ActionInfo nativeCreate(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, int productId, jint vendorId) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    ActionInfo status = ACTION_ERROR_RELEASE;
    if (LIKELY(camera)) {
        status = camera->open(productId, vendorId);
    }
    LOGD(TAG, "camera->open(): %d",status)
    return status;
}

static ActionInfo nativeAutoExposure(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jboolean isAuto) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        status = camera->autoExposure(isAuto);
    }
    LOGD(TAG, "camera->autoExposure(): %d",status)
    return status;
}

static ActionInfo nativeSetExposure(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, int level) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        if (level > 0){
            status = camera->updateExposure(level);
        } else {
            status = ACTION_ERROR_SET_EXPOSURE;
            LOGE(TAG, "camera->updateExposure() failed: level must more than 0")
        }
    }
    LOGD(TAG, "camera->updateExposure(): %d",status)
    return status;
}

static ActionInfo nativeFrameCallback(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jobject frame_callback) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        status = camera->setFrameCallback(env, frame_callback);
    }
    LOGD(TAG, "camera->setFrameCallback(): %d",status)
    return status;
}

static ActionInfo nativeFrameSize(JNIEnv *env, jobject thiz, CAMERA_ID cameraId,jint width, jint height,jint pixelFormat) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        if (width > 0 && height > 0){
            status = camera->setFrameSize(width, height, pixelFormat);
        } else {
            status = ACTION_ERROR_SET_W_H;
            LOGE(TAG, "camera->setFrameSize() failed: width and height must more than 0")
        }
    }
    LOGD(TAG, "camera->setFrameSize(): %d",status)
    return status;
}

static ActionInfo nativePreview(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jobject surface) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        ANativeWindow *window = surface ? ANativeWindow_fromSurface(env, surface) : NULL;
        status = camera->setPreview(window);
    }
    LOGD(TAG, "camera->setPreview(): %d",status)
    return status;
}

static ActionInfo nativeStart(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        status = camera->start();
    }
    LOGD(TAG, "camera->start(): %d",status)
    return status;
}

static ActionInfo nativeStop(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        status = camera->stop();
    }
    LOGD(TAG, "camera->stop(): %d",status)
    return status;
}

static ActionInfo nativeDestroy(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<Camera *>(cameraId);
    setFieldLong(env, thiz, OBJECT_ID,0);
    ActionInfo status = ACTION_ERROR_RELEASE;
    if (LIKELY(camera)) {
        status = camera->close();
        LOGD(TAG, "camera->close(): %d",status)
        status = camera->destroy();
        SAFE_DELETE(camera)
    }
    LOGD(TAG, "camera->destroy(): %d",status)
    return status;
}

static const JNINativeMethod METHODS[] = {
        {"nativeInit",             "()J",                                           (void *) nativeInit},
        {"nativeCreate",           "(JII)I",                                        (void *) nativeCreate},
        {"nativeAutoExposure",     "(JZ)I",                                         (void *) nativeAutoExposure},
        {"nativeSetExposure",      "(JI)I",                                         (void *) nativeSetExposure},
        {"nativeFrameCallback",    "(JLcom/hsj/camera/CameraAPI$IFrameCallback;)I", (void *) nativeFrameCallback},
        {"nativeFrameSize",        "(JIII)I",                                       (void *) nativeFrameSize},
        {"nativePreview",          "(JLandroid/view/Surface;)I",                    (void *) nativePreview},
        {"nativeStart",            "(J)I",                                          (void *) nativeStart},
        {"nativeStop",             "(J)I",                                          (void *) nativeStop},
        {"nativeDestroy",          "(J)I",                                          (void *) nativeDestroy},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    //获取JNIEnv
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;
    //注册函数 registerNativeMethods
    jclass clazz = env->FindClass(CLASS_NAME);
    if (clazz == nullptr) return JNI_ERR;
    jint ret = env->RegisterNatives(clazz, METHODS, sizeof(METHODS) / sizeof(JNINativeMethod));
    return ret == JNI_OK ? JNI_VERSION_1_6 : ret;
}
