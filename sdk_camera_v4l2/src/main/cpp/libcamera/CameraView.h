//
// Created by Hsj on 2021/5/31.
//

#ifndef ANDROID_CAMERA_V4L2_CAMERAVIEW_H
#define ANDROID_CAMERA_V4L2_CAMERAVIEW_H

#include "Common.h"
#include "YUVConvert.h"
#include <android/native_window_jni.h>

class CameraView {
private:
    int pixelWidth;
    int pixelHeight;
    int pixelFormat;
    int pixelStride;
    int lineSize;
    size_t pixelSize;
    YUVConvert *convert;
    ANativeWindow *window;
    void renderNV12(const uint8_t *data);
    void renderYUV422(const uint8_t *data);
    void renderYUYV(const uint8_t *data);
    void renderGray16(const uint8_t *data);
    void renderDepth(const uint8_t *data);

public:
    CameraView(int pixelWidth, int pixelHeight, PixelFormat pixelFormat, ANativeWindow *window);
    ~CameraView();
    void render(uint8_t *data);
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_CAMERAVIEW_H
