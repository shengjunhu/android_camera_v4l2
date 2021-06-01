//
// Created by Hsj on 2021/1/22.
//

#ifndef ANDROID_CAMERA_V4L2_CAMERA_H
#define ANDROID_CAMERA_V4L2_CAMERA_H

#include <pthread.h>
#include <android/native_window_jni.h>
#include "NativeAPI.h"
#include "DecodeCreator.h"

enum StatusInfo{
    STATUS_READY    = 0,
    STATUS_CREATE   = 1,
    STATUS_PARAM    = 2,
    STATUS_OPEN     = 3,
    STATUS_START    = 4,
};

struct VideoBuffer {
    void *start;
    size_t length;
};

class Camera {
private:
    int fd;
    int frameWidth;
    int frameHeight;
    int pixelFormat;

    size_t pixelBytes;
    uint8_t *outBuffer;
    VideoBuffer *buffers;
    DecodeCreator *decoder;

    ANativeWindow *preview;
    jobject frameCallback;
    jmethodID frameCallback_onFrame;

    const char* deviceName;
    pthread_t thread_camera;
    volatile StatusInfo status;

    ActionInfo prepareBuffer();
    static void *loopThread(void *args);
    void loopFrame(JNIEnv *env, Camera *camera);
    void sendFrame(JNIEnv *env, uint8_t *data);

public:
    Camera();
    ~Camera();
    inline const StatusInfo getStatus() const;
    ActionInfo open(unsigned int pid, unsigned int vid);
    ActionInfo autoExposure(bool isAuto);
    ActionInfo updateExposure(unsigned int level);
    ActionInfo setFrameSize(int width, int height, int pixel_format);
    ActionInfo setFrameCallback(JNIEnv *env, jobject frame_callback);
    ActionInfo setPreview(ANativeWindow *window);
    ActionInfo start();
    ActionInfo stop();
    ActionInfo close();
    ActionInfo destroy();
};

#endif //ANDROID_CAMERA_V4L2_CAMERA_H