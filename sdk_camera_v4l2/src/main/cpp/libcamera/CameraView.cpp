//
// Created by Hsj on 2021/5/31.
//

#include <cstring>
#include <malloc.h>
#include "Common.h"
#include "CameraView.h"

#define TAG "CameraView"
#define HIST_SIZE 0xFFFF
#define XN_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define XN_MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef int XnInt32;
typedef short XnInt16;
typedef unsigned char XnUInt8;
typedef uint16_t DepthPixel;
typedef uint16_t Gray16Pixel;
typedef struct {
    uint8_t y1;
    uint8_t u;
    uint8_t y2;
    uint8_t v;
} YUYV;

unsigned int *histogram;

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
        pixelStride = pixelWidth * 2;
    } else if (pixelFormat == PIXEL_FORMAT_GRAY16) {
        lineSize = pixelWidth * 4;
        pixelStride = pixelWidth * 2;
        pixelSize = pixelWidth * pixelHeight * 2;
    } else if (pixelFormat == PIXEL_FORMAT_DEPTH) {
        lineSize = pixelWidth * 4;
        pixelStride = pixelWidth * 2;
        pixelSize = pixelWidth * pixelHeight * 2;
        histogram = (unsigned int *) malloc(HIST_SIZE * sizeof(unsigned int));
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
    if (window) ANativeWindow_release(window);
    if (convert) {
        convert->destroy();
        SAFE_DELETE(convert)
    }
    SAFE_FREE(histogram)
    window = nullptr;
    pixelSize = 0;
    pixelWidth = 0;
    pixelHeight = 0;
    pixelFormat = 0;
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
        case PIXEL_FORMAT_GRAY16:
            renderGray16(data);
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

//==================================================================================================

static void neon_memcpy(uint8_t *dst, uint8_t *src, size_t sz){
    if (sz & 63) sz = (sz & -64) + 64;
    asm volatile (
    "NEONCopyPLD: \n"
    " VLDM %[src]!,{d0-d7} \n"
    " VSTM %[dst]!,{d0-d7} \n"
    " SUBS %[sz],%[sz],#0x40 \n"
    " BGT NEONCopyPLD \n"
    : [dst]"+r"(dst), [src]"+r"(src), [sz]"+r"(sz) : : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}

static void copyFrame(const uint8_t *src, uint8_t *dest, const int width, int height,
                      const int src_stride, const int dest_stride) {
    const int h8 = height % 8;
    for (int i = 0; i < h8; i++) {
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
    }
    for (int i = 0; i < height; i += 8) {
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
        memcpy(dest, src, width);
        dest += dest_stride;
        src += src_stride;
    }
}

static void YUVtoRGB888(XnUInt8 y, XnUInt8 u, XnUInt8 v, XnUInt8 &r, XnUInt8 &g, XnUInt8 &b) {
    XnInt32 nC = y - 16;
    XnInt16 nD = u - 128;
    XnInt16 nE = v - 128;
    nC = nC * 298 + 128;
    r = (XnUInt8) XN_MIN(XN_MAX((nC + 409 * nE) >> 8, 0), 0xff);
    g = (XnUInt8) XN_MIN(XN_MAX((nC - 100 * nD - 208 * nE) >> 8, 0), 0xff);
    b = (XnUInt8) XN_MIN(XN_MAX((nC + 516 * nD) >> 8, 0), 0xff);
}

static void calculateDepthHist(const DepthPixel *depth, const unsigned long size) {
    unsigned int value = 0;
    unsigned int index = 0;
    unsigned int numberOfPoints = 0;
    // Calculate the accumulative histogram
    memset(histogram, 0, HIST_SIZE * sizeof(int));
    for (int i = 0; i < size/sizeof(DepthPixel); ++i, ++depth) {
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
            histogram[index] = (unsigned int) (256 * (1.0f - ((float) histogram[index] / numberOfPoints)));
        }
    }
}

//==================================================================================================

//10ms
void CameraView::renderNV12(const uint8_t *data) {
    uint8_t *rgba = convert->convertNV12(data);
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        //6ms
        uint64_t start = timeMs();
        auto *dest = (uint8_t *) buffer.bits;
        memcpy(dest, rgba, pixelSize);
        //neon_memcpy(dest, rgba, pixelSize);
        LOGD(TAG,"time=%lld", timeMs()-start)
        //3ms
        ANativeWindow_unlockAndPost(window);
    }
}

//10ms
void CameraView::renderYUV422(const uint8_t *data) {
    unsigned char *rgba = convert->convertYUV422(data);
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        //6ms
        auto *dest = (unsigned char *)buffer.bits;
        memcpy(dest, rgba, pixelSize);
        //3ms
        ANativeWindow_unlockAndPost(window);
    }
}

//20ms
void CameraView::renderYUYV(const uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        int pixelWidth2 = pixelWidth / 2;
        auto *dest = (uint8_t *) buffer.bits;
        for (int h = 0; h < pixelHeight; ++h) {
            uint8_t *texture = dest + h * lineSize;
            const auto *yuyv = (const YUYV *) (data + h * pixelStride);
            for (int w = 0; w < pixelWidth2; ++w, ++yuyv) {
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

//20ms
void CameraView::renderGray16(const uint8_t *data) {
    // 1-Find max gray value
    int maxGray = 0;
    size_t length = pixelSize / sizeof(Gray16Pixel);
    const auto *_data = (const Gray16Pixel *) data;
    for (int i = 0; i < length; ++i, ++_data) {
        if (*_data > maxGray) {
            maxGray = *_data;
        }
    }
    // 2-Update texture
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        auto *dest = (uint8_t *) buffer.bits;
        for (int h = 0; h < pixelHeight; ++h) {
            uint8_t *texture = dest + h * lineSize;
            const auto *gray = (const Gray16Pixel *) (data + h * pixelStride);
            for (int w = 0; w < pixelWidth; ++w, ++gray, texture += 4) {
                texture[0] = texture[1] = texture[2] = 255.0 * (*gray) / maxGray;
                texture[3] = 0xff;
            }
        }
        ANativeWindow_unlockAndPost(window);
    }
}

//20ms
void CameraView::renderDepth(const uint8_t *data) {
    // 1-Calculate Depth
    calculateDepthHist((const DepthPixel *)data, pixelSize);
    // 2-Update texture
    ANativeWindow_Buffer buffer;
    if (LIKELY(ANativeWindow_lock(window, &buffer, nullptr) == 0)) {
        auto *dest = (uint8_t *) buffer.bits;
        for (int h = 0; h < pixelHeight; ++h) {
            uint8_t *texture = dest + h * lineSize;
            const auto *depth = (const DepthPixel *) (data + h * pixelStride);
            for (int w = 0; w < pixelWidth; ++w, ++depth, texture += 4) {
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


