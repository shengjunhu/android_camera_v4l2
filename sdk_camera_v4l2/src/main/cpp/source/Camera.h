//
// Created by Hsj on 2021/1/22.
//

#ifndef ANDROID_CAMERA_V4L2_CAMERA_H
#define ANDROID_CAMERA_V4L2_CAMERA_H

#include <pthread.h>
#include "NativeAPI.h"
#include "HwDecoder.h"

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
    unsigned int frameWidth;
    unsigned int frameHeight;
    unsigned int pixelFormat;
    VideoBuffer *buffers;
    HwDecoder *hwDecoder;
    jobject frameCallback;
    ANativeWindow *preview;
    const char* deviceName;
    volatile StatusInfo status;
    pthread_t thread_camera;
    ActionInfo prepareBuffer();
    static void *loopFrame(void *args);

public:
    Camera();
    ~Camera();
    inline const StatusInfo getStatus() const;
    ActionInfo open(unsigned int pid, unsigned int vid);
    ActionInfo autoExposure(bool isAuto);
    ActionInfo updateExposure(unsigned int level);
    ActionInfo setFrameSize(unsigned int width, unsigned int height, unsigned int pixel_format);
    ActionInfo setFrameCallback(JNIEnv *env, jobject frame_callback);
    ActionInfo setPreview(ANativeWindow *window);
    ActionInfo start();
    ActionInfo stop();
    ActionInfo close();
    ActionInfo destroy();
};

#endif //ANDROID_CAMERA_V4L2_CAMERA_H