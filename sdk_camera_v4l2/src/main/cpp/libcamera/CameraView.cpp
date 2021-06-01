//
// Created by Hsj on 2021/5/31.
//

#include "Common.h"
#include "CameraView.h"

#define TAG "CameraView"
#define XN_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define XN_MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB888;

CameraView::CameraView(int pixelWidth, int pixelHeight,
        PixelFormat pixelFormat, ANativeWindow *window) :
        window(window),
        pixelWidth(pixelWidth),
        pixelHeight(pixelHeight),
        pixelFormat(pixelFormat) {
    ANativeWindow_setBuffersGeometry(window, pixelWidth, pixelHeight,WINDOW_FORMAT_RGBA_8888);
}

CameraView::~CameraView() {
    destroy();
}

void CameraView::destroy() {
    if (window) {
        ANativeWindow_release(window);
    }
    window = NULL;
    pixelWidth = 0;
    pixelHeight = 0;
    pixelFormat = 0;
}

void CameraView::update(uint8_t *data) {
    switch (pixelFormat) {
        case PIXEL_FORMAT_RGB:
            renderRGB(data);
            break;
        case PIXEL_FORMAT_RGBA:
            renderRGBA(data);
            break;
        case PIXEL_FORMAT_NV12:
            renderNV12(data);
            break;
        case PIXEL_FORMAT_YUYV:
            renderYUYV(data);
            break;
        case PIXEL_FORMAT_GRAY:
            renderGray(data);
            break;
        case PIXEL_FORMAT_DEPTH:
            renderDepth(data);
            break;
        case PIXEL_FORMAT_ERROR:
        default:
            LOGE(TAG,"error format: %d",pixelFormat)
            break;
    }
}

void CameraView::renderRGB(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, NULL) == 0)) {
        auto *dest = (uint8_t *) buffer.bits;
        for (int y = 0; y < pixelHeight; ++y) {
            uint8_t *texture = dest + y * buffer.width * 4;
            const auto *_data = (const RGB888 *) (data + y * pixelWidth);
            for (int x = 0; x < pixelWidth; ++x, ++_data, texture += 4) {
                texture[0] = _data->r;
                texture[1] = _data->g;
                texture[2] = _data->b;
                texture[3] = 0xff;
            }
        }
        ANativeWindow_unlockAndPost(window);
    }
}

void CameraView::renderRGBA(const uint8_t *data) {}

void CameraView::renderNV12(const uint8_t *data) {}

void CameraView::renderYUYV(const uint8_t *data) {}

void CameraView::renderGray(const uint8_t *data) {}

void CameraView::renderDepth(const uint8_t *data) {}


