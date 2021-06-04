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
    uint8_t u;
    uint8_t y1;
    uint8_t v;
    uint8_t y2;
} YUV422;

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
    if (pixelFormat == PIXEL_FORMAT_YUYV) {
        lineSize = pixelWidth * 4;
        pixelStride = pixelWidth * 1;
    } else if (pixelFormat == PIXEL_FORMAT_GRAY) {
        pixelStride = pixelWidth * 2;
    } else if (pixelFormat == PIXEL_FORMAT_DEPTH) {
        pixelStride = pixelWidth * 2;
    } else {
        pixelSize = pixelWidth * pixelHeight * 4;
        convert = new YUVConvert(pixelWidth, pixelHeight, pixelFormat);
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
    if (convert) {
        convert->destroy();
        SAFE_DELETE(convert)
    }
    if (window) ANativeWindow_release(window);
}

void CameraView::render(uint8_t *data) {
    switch (pixelFormat) {
        case PIXEL_FORMAT_NV12:
            renderNV12(data);
            break;
        case PIXEL_FORMAT_YUV422:
            renderYUV422(data);
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
            LOGE(TAG, "Render pixelFormat is error: %d", pixelFormat)
            break;
    }
}

void CameraView::renderNV12(const uint8_t *data) {
    unsigned char *rgba = convert->convertNV12(data);
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        //5~6ms
        memcpy(buffer.bits, rgba, pixelSize);
        //2~3ms
        ANativeWindow_unlockAndPost(window);
    }
}

void CameraView::renderYUV422(const uint8_t *data) {
    unsigned char *rgba = convert->convertYUV422(data);
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        //5~6ms
        memcpy(buffer.bits, rgba, pixelSize);
        //2~3ms
        ANativeWindow_unlockAndPost(window);
    }
}

void CameraView::renderYUYV(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, NULL) == 0)) {
        int pixelWidth2 = pixelWidth / 2;
        auto *dest = (uint8_t *) buffer.bits;
        for (int h = 0; h < pixelHeight; ++h) {
            uint8_t *texture = dest + h * lineSize;
            const auto *yuyv = (const YUV422 *) (data + h * pixelStride);
            for (int w = 0; w < pixelWidth2; ++w, ++yuyv, texture += 4) {
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


