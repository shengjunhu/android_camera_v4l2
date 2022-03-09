//
// Created by Hsj on 2021/5/31.
//

#ifndef ANDROID_CAMERA_V4L2_CAMERAAPI_H
#define ANDROID_CAMERA_V4L2_CAMERAAPI_H

#include <vector>
#include <pthread.h>
#include "NativeAPI.h"
#include "CameraView.h"
#include "DecoderFactory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATUS_CREATE   = 0,
    STATUS_OPEN     = 1,
    STATUS_INIT     = 2,
    STATUS_RUN      = 3,
}StatusInfo;

typedef enum {
    FRAME_FORMAT_MJPEG = 0,
    FRAME_FORMAT_YUYV  = 1,
    FRAME_FORMAT_DEPTH = 2,
} FrameFormat;

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
    uint8_t* out_buffer;
    VideoBuffer* buffers;
    DecoderFactory* decoder;

    CameraView *preview;
    jobject frameCallback;
    jmethodID frameCallback_onFrame;

    pthread_t thread_camera;
    volatile StatusInfo status;
    inline const StatusInfo getStatus() const;

    ActionInfo prepareBuffer();
    static void* loopThread(void *args);
    void loopFrame(JNIEnv *env, CameraAPI *camera);
    void sendFrame(JNIEnv *env, uint8_t *data);
    void renderFrame(uint8_t *data);

public:
    CameraAPI();
    ~CameraAPI();
    ActionInfo connect(unsigned int pid, unsigned int vid);
    ActionInfo autoExposure(bool isAuto);
    ActionInfo updateExposure(unsigned int level);
    ActionInfo getSupportSize(std::vector<std::pair<int, int>> &sizes);
    ActionInfo setFrameSize(int width, int height, int frame_format);
    ActionInfo setFrameCallback(JNIEnv *env, jobject frame_callback);
    ActionInfo setPreview(ANativeWindow *window);
    ActionInfo start();
    ActionInfo stop();
    ActionInfo close();
    ActionInfo destroy();
};

#ifdef __cplusplus
}  // extern "C"
#endif

#endif //ANDROID_CAMERA_V4L2_CAMERAAPI_H
