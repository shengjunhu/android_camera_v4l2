//
// Created by Hsj on 2021/5/31.
//

#include <cstring>
#include "Common.h"
#include "CameraView.h"

#define TAG "CameraView"
#define XN_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define XN_MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef int XnInt32;
typedef short XnInt16;
typedef unsigned char XnUInt8;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB888;

typedef struct {
    uint8_t y1;
    uint8_t u;
    uint8_t y2;
    uint8_t v;
} YUYV;

typedef struct {
    uint8_t y;
    uint8_t v;
    uint8_t u;
} NV12;

static void YUVtoRGB888(XnUInt8 y, XnUInt8 u, XnUInt8 v, XnUInt8 &r, XnUInt8 &g, XnUInt8 &b) {
    XnInt32 nC = y - 16;
    XnInt16 nD = u - 128;
    XnInt16 nE = v - 128;
    nC = nC * 298 + 128;
    r = (XnUInt8) XN_MIN(XN_MAX((nC + 409 * nE) >> 8, 0), 255);
    g = (XnUInt8) XN_MIN(XN_MAX((nC - 100 * nD - 208 * nE) >> 8, 0), 255);
    b = (XnUInt8) XN_MIN(XN_MAX((nC + 516 * nD) >> 8, 0), 255);
}

CameraView::CameraView(int pixelWidth, int pixelHeight,
        PixelFormat pixelFormat, ANativeWindow *window) :
        window(window),
        pixelSize(0),
        pixelStride(0),
        pixelWidth(pixelWidth),
        pixelHeight(pixelHeight),
        pixelFormat(pixelFormat),
        lineSize(pixelWidth * 4) {
    if (pixelFormat == PIXEL_FORMAT_RGB) {
        lineSize = pixelWidth * 4;
        pixelStride = pixelWidth * 3;
        pixelSize = pixelWidth * pixelHeight * 3;
    } else if (pixelFormat == PIXEL_FORMAT_NV12) {
        pixelStride = pixelWidth;
        pixelSize = pixelWidth * pixelHeight * 3 / 2;
    } else if (pixelFormat == PIXEL_FORMAT_YUYV) {
        pixelStride = pixelWidth * 2;
        pixelSize = pixelWidth * pixelHeight * 2;
    } else if (pixelFormat == PIXEL_FORMAT_GRAY) {
        pixelStride = pixelWidth * 2;
        pixelSize = pixelWidth * pixelHeight * 2;
    } else if (pixelFormat == PIXEL_FORMAT_DEPTH) {
        pixelStride = pixelWidth * 2;
        pixelSize = pixelWidth * pixelHeight * 2;
    } else { //Default RGBA
        pixelStride = pixelWidth * 4;
        pixelSize = pixelWidth * pixelHeight * 4;
    }
    ANativeWindow_setBuffersGeometry(window, pixelWidth, pixelHeight, WINDOW_FORMAT_RGBA_8888);
}

CameraView::~CameraView() {
    destroy();
}

void CameraView::destroy() {
    window = nullptr;
    pixelSize = 0;
    pixelWidth = 0;
    pixelHeight = 0;
    pixelFormat = 0;
    if (window) ANativeWindow_release(window);
}

void CameraView::render(uint8_t *data) {
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
            LOGE(TAG, "error format: %d", pixelFormat)
            break;
    }
}

void CameraView::renderRGB(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        auto *dest = (uint8_t *) buffer.bits;
        for (int h = 0; h < pixelHeight; ++h) {
            uint8_t *texture = dest + h * lineSize;
            auto *rgb = (RGB888 *) (data + h * pixelStride);
            for (int w = 0; w < pixelWidth; ++w, ++rgb, texture += 4) {
                texture[0] = rgb->r;
                texture[1] = rgb->g;
                texture[2] = rgb->b;
                texture[3] = 0xff;
            }
        }
        ANativeWindow_unlockAndPost(window);
    }
}

void CameraView::renderRGBA(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        memcpy(buffer.bits, data, pixelSize);
        ANativeWindow_unlockAndPost(window);
    }
}

void CameraView::renderNV12(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        long size = pixelWidth * pixelHeight;
        const uint8_t *y = data + size;
        const uint8_t *vu = y + size / 2;
        auto *dest = (uint8_t *) buffer.bits;
        for (int h = 0; h < pixelHeight; ++h) {
            uint8_t *texture = dest + h * lineSize;
            const auto *nv12 = (const NV12 *) (y + h * pixelStride);
            for (int w = 0; w < pixelWidth; ++w, ++nv12, texture += 4) {
                // YUVtoRGB888(yuyv->y1, yuyv->u, yuyv->v, texture[0], texture[1],texture[2]);
                texture[3] = 0xff;
            }
        }
        ANativeWindow_unlockAndPost(window);
    }
}

void CameraView::renderYUYV(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        auto *dest = (uint8_t *) buffer.bits;
        for (int h = 0; h < pixelHeight; ++h) {
            uint8_t *texture = dest + h * lineSize;
            const auto *yuyv = (const YUYV *) (data + h * pixelStride);
            for (int w = 0; w < pixelWidth / 2; ++w, ++yuyv, texture += 4) {
                YUVtoRGB888(yuyv->y1, yuyv->u, yuyv->v, texture[0], texture[1], texture[2]);
                texture[3] = 0xff;
                texture += 4;
                YUVtoRGB888(yuyv->y2, yuyv->u, yuyv->v, texture[0], texture[1], texture[2]);
                texture[3] = 0xff;
                texture += 4;
            }
        }
        ANativeWindow_unlockAndPost(window);
    }
}

void CameraView::renderGray(const uint8_t *data) {}

void CameraView::renderDepth(const uint8_t *data) {}


