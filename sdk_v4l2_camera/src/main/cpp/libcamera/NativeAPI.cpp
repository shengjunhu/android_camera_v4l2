//
// Created by Hsj on 2021/4/26.
//

#include "Common.h"
#include "NativeAPI.h"
#include "CameraAPI.h"

#define TAG "NativeAPI"
#define OBJECT_ID "nativeObj"
#define CLASS_NAME "com/hsj/camera/CameraAPI"

typedef jlong CAMERA_ID;

static void setFieldLong(JNIEnv *env, jobject obj, const char *fieldName, jlong value) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID field = env->GetFieldID(clazz, fieldName, "J");
    if (LIKELY(field)) {
        env->SetLongField(obj, field, value);
    } else {
        LOGE(TAG, "setFieldLong: failed '%s' not found", fieldName);
    }
    env->DeleteLocalRef(clazz);
}

static CAMERA_ID nativeInit(JNIEnv *env, jobject thiz) {
    auto *camera = new CameraAPI();
    auto cameraId = reinterpret_cast<CAMERA_ID>(camera);
    setFieldLong(env, thiz, OBJECT_ID, cameraId);
    return cameraId;
}

static ActionInfo nativeCreate(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, int productId, jint vendorId) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    ActionInfo status = ACTION_ERROR_RELEASE;
    if (LIKELY(camera)) {
        status = camera->connect(productId, vendorId);
    }
    LOGD(TAG, "camera->open(): %d", status);
    return status;
}

static ActionInfo nativeAutoExposure(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jboolean isAuto) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        status = camera->autoExposure(isAuto);
    }
    LOGD(TAG, "camera->autoExposure(): %d", status);
    return status;
}

static ActionInfo nativeSetExposure(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, int level) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        if (level > 0) {
            status = camera->updateExposure(level);
        } else {
            status = ACTION_ERROR_SET_EXPOSURE;
            LOGE(TAG, "camera->updateExposure() failed: level must more than 0");
        }
    }
    LOGD(TAG, "camera->updateExposure(): %d", status);
    return status;
}

static jobjectArray nativeSupportSize(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    jobjectArray objArr = nullptr;
    if (LIKELY(camera)) {
        std::vector<std::pair<int, int>> sizes;
        ActionInfo status = camera->getSupportSize(sizes);
        if (status == ACTION_SUCCESS) {
            int length = sizes.size();
            if (length > 0){
                jclass cls = env->FindClass("[I");
                objArr = env->NewObjectArray(length, cls,nullptr);
                for(size_t i = 0; i < length; i++) {
                    jint size[2] = {sizes[i].first, sizes[i].second};
                    jintArray arr = env->NewIntArray(2);
                    env->SetIntArrayRegion(arr, 0, 2, size);
                    env->SetObjectArrayElement(objArr, i, arr);
                    env->DeleteLocalRef(arr);
                }
            } else {
                LOGE(TAG, "camera->getSupportSize(): empty.");
            }
            LOGI(TAG, "camera->getSupportSize(): length:%d", length);
        } else {
            LOGE(TAG, "camera->getSupportSize(): status:%d", status);
        }
        std::vector<std::pair<int, int>>().swap(sizes);
    }
    return objArr;
}

static ActionInfo nativeFrameSize(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jint width, jint height, jint frameFormat) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        if (width > 0 && height > 0) {
            status = camera->setFrameSize(width, height, frameFormat);
        } else {
            status = ACTION_ERROR_SET_W_H;
            LOGE(TAG, "camera->setFrameSize() failed: width and height must more than 0");
        }
    }
    LOGD(TAG, "camera->setFrameSize(): %d", status);
    return status;
}

static ActionInfo nativeFrameCallback(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jobject frame_callback) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        jobject _frame_callback = env->NewGlobalRef(frame_callback);
        status = camera->setFrameCallback(env, _frame_callback);
    }
    LOGD(TAG, "camera->setFrameCallback(): %d", status);
    return status;
}

static ActionInfo nativePreview(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jobject surface) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        status = camera->setPreview(surface ? ANativeWindow_fromSurface(env, surface) : NULL);
    }
    LOGD(TAG, "camera->setPreview(): %d", status);
    return status;
}

static ActionInfo nativeStart(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        status = camera->start();
    }
    LOGD(TAG, "camera->start(): %d", status);
    return status;
}

static ActionInfo nativeStop(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    ActionInfo status = ACTION_ERROR_DESTROY;
    if (LIKELY(camera)) {
        status = camera->stop();
    }
    LOGD(TAG, "camera->stop(): %d", status);
    return status;
}

static ActionInfo nativeDestroy(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<CameraAPI *>(cameraId);
    setFieldLong(env, thiz, OBJECT_ID, 0);
    ActionInfo status = ACTION_ERROR_RELEASE;
    if (LIKELY(camera)) {
        status = camera->close();
        LOGD(TAG, "camera->close(): %d", status);
        status = camera->destroy();
        SAFE_DELETE(camera)
    }
    LOGD(TAG, "camera->destroy(): %d", status);
    return status;
}

static const JNINativeMethod METHODS[] = {
        {"nativeInit",          "()J",                                 (void *) nativeInit},
        {"nativeCreate",        "(JII)I",                              (void *) nativeCreate},
        {"nativeAutoExposure",  "(JZ)I",                               (void *) nativeAutoExposure},
        {"nativeSetExposure",   "(JI)I",                               (void *) nativeSetExposure},
        {"nativeFrameCallback", "(JLcom/hsj/camera/IFrameCallback;)I", (void *) nativeFrameCallback},
        {"nativeSupportSize",   "(J)[[I",                              (void *) nativeSupportSize},
        {"nativeFrameSize",     "(JIII)I",                             (void *) nativeFrameSize},
        {"nativePreview",       "(JLandroid/view/Surface;)I",          (void *) nativePreview},
        {"nativeStart",         "(J)I",                                (void *) nativeStart},
        {"nativeStop",          "(J)I",                                (void *) nativeStop},
        {"nativeDestroy",       "(J)I",                                (void *) nativeDestroy},
};

jint registerAPI(JNIEnv *env){
    jclass clazz = env->FindClass(CLASS_NAME);
    if (clazz == nullptr) return JNI_ERR;
    jint ret = env->RegisterNatives(clazz, METHODS, sizeof(METHODS) / sizeof(JNINativeMethod));
    return ret == JNI_OK ? JNI_VERSION_1_6 : ret;
}
