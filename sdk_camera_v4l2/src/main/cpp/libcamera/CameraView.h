//
// Created by Hsj on 2021/5/31.
//

#ifndef ANDROID_CAMERA_V4L2_CAMERAVIEW_H
#define ANDROID_CAMERA_V4L2_CAMERAVIEW_H

#include "Common.h"
#include "DecoderFactory.h"
#include <android/native_window_jni.h>

class CameraView {
private:
    int pixelWidth;
    int pixelHeight;
    int pixelFormat;
    int stride_width;
    int stride_rgba;
    int stride_uv;
    int start_uv;
    int start_u;
    int start_v;
    uint8_t *yuv422;
    size_t frameSize;
    ANativeWindow *window;
    void renderNV12(const uint8_t *data);
    void renderYUV422(const uint8_t *data);
    void renderYUYV(const uint8_t *data);
    void renderDepth(const uint8_t *data);

public:
    CameraView(int pixelWidth, int pixelHeight, PixelFormat pixelFormat, ANativeWindow *window);
    ~CameraView();
    void render(uint8_t *data);
    void stop();
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_CAMERAVIEW_H
