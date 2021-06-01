//
// Created by Hsj on 2021/5/31.
//

#ifndef ANDROID_CAMERA_V4L2_CAMERAVIEW_H
#define ANDROID_CAMERA_V4L2_CAMERAVIEW_H

#include <android/native_window_jni.h>

typedef enum {
    PIXEL_FORMAT_RGB   = 0,
    PIXEL_FORMAT_RGBA  = 1,
    PIXEL_FORMAT_NV12  = 2,
    PIXEL_FORMAT_YUYV  = 3,
    PIXEL_FORMAT_GRAY  = 4,
    PIXEL_FORMAT_DEPTH = 5,
    PIXEL_FORMAT_ERROR = 6,
} PixelFormat;

class CameraView {
private:
    int pixelWidth;
    int pixelHeight;
    int pixelFormat;
    ANativeWindow *window;
    void renderRGB(const uint8_t *data);
    void renderRGBA(const uint8_t *data);
    void renderNV12(const uint8_t *data);
    void renderYUYV(const uint8_t *data);
    void renderGray(const uint8_t *data);
    void renderDepth(const uint8_t *data);

public:
    CameraView(int pixelWidth, int pixelHeight, PixelFormat pixelFormat, ANativeWindow *window);
    ~CameraView();
    void update(uint8_t *data);
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_CAMERAVIEW_H
