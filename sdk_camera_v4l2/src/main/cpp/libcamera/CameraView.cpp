//
// Created by Hsj on 2021/5/31.
//

#include <cstring>
#include <malloc.h>
#include <libyuv.h>
#include "Common.h"
#include "CameraView.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "CameraView"
#define HIST_SIZE 0xFFFF

typedef uint16_t DepthPixel;

static unsigned int *histogram;

static void calculateDepthHist(const DepthPixel *depth, const unsigned long size) {
    unsigned int value = 0;
    unsigned int index = 0;
    unsigned int numberOfPoints = 0;
    // Calculate the accumulative histogram
    memset(histogram, 0, HIST_SIZE * sizeof(int));
    for (int i = 0; i < size / sizeof(DepthPixel); ++i, ++depth) {
        value = *depth;
        if (value != 0) {
            histogram[value]++;
            numberOfPoints++;
        }
    }
    for (index = 1; index < HIST_SIZE; index++) {
        histogram[index] += histogram[index - 1];
    }
    if (numberOfPoints != 0) {
        for (index = 1; index < HIST_SIZE; index++) {
            histogram[index] = (unsigned int) (256 * (1.0f -((float) histogram[index] / numberOfPoints)));
        }
    }
}

//==================================================================================================

CameraView::CameraView(int pixelWidth, int pixelHeight,
        PixelFormat pixelFormat, ANativeWindow *window) :
        yuv422(NULL),
        window(window),
        pixelWidth(pixelWidth),
        pixelHeight(pixelHeight),
        pixelFormat(pixelFormat),
        stride_width(pixelWidth * 2) {
    if (pixelFormat == PIXEL_FORMAT_NV12) {
        start_uv = pixelWidth * pixelHeight;
    } else if (pixelFormat == PIXEL_FORMAT_YUV422) {
        stride_uv = pixelWidth / 2;
        start_u = pixelWidth * pixelHeight;
        start_v = start_u * 3 / 2;
    } else if (pixelFormat == PIXEL_FORMAT_YUYV) {
        stride_uv = pixelWidth / 2;
        start_u = pixelWidth * pixelHeight;
        start_v = start_u * 3 / 2;
        yuv422 = (uint8_t *) malloc(pixelWidth * pixelHeight * 2);
    } else if (pixelFormat == PIXEL_FORMAT_DEPTH) {
        frameSize = pixelWidth * pixelHeight * 2;
        histogram = (unsigned int *) malloc(HIST_SIZE * sizeof(unsigned int));
    } else {
        LOGE(TAG, "PixelFormat error: %d", pixelFormat);
    }
    ANativeWindow_setBuffersGeometry(window, pixelWidth, pixelHeight, WINDOW_FORMAT_RGBA_8888);
}

CameraView::~CameraView() {
    destroy();
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
        case PIXEL_FORMAT_DEPTH:
            renderDepth(data);
            break;
        case PIXEL_FORMAT_ERROR:
        default:
            LOGE(TAG, "Render pixelFormat is error: %d", pixelFormat);
            break;
    }
}

void CameraView::pause() {
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        auto *dest = (uint8_t *) buffer.bits;
        const size_t size_line = buffer.width * 4;
        const int size_stride = buffer.stride * 4;
        for (int i = 0; i < buffer.height; i++) {
            memset(dest, 0, size_line);
            dest += size_stride;
        }
        ANativeWindow_unlockAndPost(window);
    }
}

void CameraView::destroy() {
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
    SAFE_FREE(yuv422)
    SAFE_FREE(histogram)
    pixelWidth = 0;
    pixelHeight = 0;
    pixelFormat = 0;
    stride_width = 0;
    stride_uv = 0;
    frameSize = 0;
    stride_uv = 0;
    start_uv = 0;
    start_u = 0;
    start_v = 0;
}

//==================================================================================================

//NV12:10ms
void CameraView::renderNV12(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(0 == ANativeWindow_lock(window, &buffer, nullptr))) {
        auto *dest = (uint8_t *) buffer.bits;
        libyuv::NV12ToABGR(data, buffer.width,
                           data + start_uv, buffer.width,
                           dest, buffer.stride * 4,
                           buffer.width, buffer.height);
        ANativeWindow_unlockAndPost(window);
    }
}

//YUV422:10ms (YUV)
void CameraView::renderYUV422(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(0 == ANativeWindow_lock(window, &buffer, nullptr))) {
        auto *dest = (uint8_t *) buffer.bits;
        libyuv::I422ToABGR(data, buffer.width,
                           data + start_u, stride_uv,
                           data + start_v, stride_uv,
                           dest, buffer.stride * 4,
                           buffer.width, buffer.height);
        ANativeWindow_unlockAndPost(window);
    }
}

//YUYV: 18ms (YUV422)
void CameraView::renderYUYV(const uint8_t *data) {
    libyuv::YUY2ToI422(data, stride_width,
                       yuv422, pixelWidth,
                       yuv422 + start_u, stride_uv,
                       yuv422 + start_v, stride_uv,
                       pixelWidth, pixelHeight);
    ANativeWindow_Buffer buffer;
    if (LIKELY(0 == ANativeWindow_lock(window, &buffer, nullptr))) {
        auto *dest = (uint8_t *) buffer.bits;
        libyuv::I422ToABGR(yuv422, buffer.width,
                           yuv422 + start_u, stride_uv,
                           yuv422 + start_v, stride_uv,
                           dest, buffer.stride * 4,
                           buffer.width, buffer.height);
        ANativeWindow_unlockAndPost(window);
    }
}

//DEPTH16: 20ms
void CameraView::renderDepth(const uint8_t *data) {
    // 1-Calculate Depth
    calculateDepthHist((const DepthPixel *) data, frameSize);
    // 2-Update texture
    ANativeWindow_Buffer buffer;
    if (LIKELY(0 == ANativeWindow_lock(window, &buffer, nullptr))) {
        auto *dest = (uint8_t *) buffer.bits;
        for (int h = 0; h < buffer.height; ++h) {
            uint8_t *texture = dest + h * buffer.stride * 4;
            const auto *depth = (const DepthPixel *) (data + h * stride_width);
            for (int w = 0; w < buffer.width; ++w, ++depth, texture += 4) {
                unsigned int val = histogram[*depth];
                texture[0] = val;
                texture[1] = val;
                texture[2] = val;
                texture[3] = 0xff;
            }
        }
        ANativeWindow_unlockAndPost(window);
    }
}

#ifdef __cplusplus
}  // extern "C"
#endif