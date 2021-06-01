//
// Created by Hsj on 2021/5/31.
//

#ifndef ANDROID_CAMERA_V4L2_CAMERAAPI_H
#define ANDROID_CAMERA_V4L2_CAMERAAPI_H

#include <pthread.h>
#include "NativeAPI.h"
#include "CameraView.h"
#include "DecodeCreator.h"

enum StatusInfo {
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

class CameraAPI {
private:
    int fd;
    int frameWidth;
    int frameHeight;
    int frameFormat;

    size_t pixelBytes;
    uint8_t *outBuffer;
    VideoBuffer *buffers;
    DecodeCreator *decoder;

    CameraView *preview;
    jobject frameCallback;
    jmethodID frameCallback_onFrame;

    const char* deviceName;
    pthread_t thread_camera;
    volatile StatusInfo status;

    ActionInfo prepareBuffer();
    static void *loopThread(void *args);
    void loopFrame(JNIEnv *env, CameraAPI *camera);
    void sendFrame(JNIEnv *env, uint8_t *data);
    void renderFrame(uint8_t *data);

public:
    CameraAPI();
    ~CameraAPI();
    inline const StatusInfo getStatus() const;
    ActionInfo open(unsigned int pid, unsigned int vid);
    ActionInfo autoExposure(bool isAuto);
    ActionInfo updateExposure(unsigned int level);
    ActionInfo setFrameSize(int width, int height, int frame_format);
    ActionInfo setFrameCallback(JNIEnv *env, jobject frame_callback);
    ActionInfo setPreview(ANativeWindow *window);
    ActionInfo start();
    ActionInfo stop();
    ActionInfo close();
    ActionInfo destroy();
};

#endif //ANDROID_CAMERA_V4L2_CAMERAAPI_H
