//
// Created by Hsj on 2021/6/4.
//

#include <libyuv.h>
#include "Common.h"
#include "YUVConvert.h"

#define TAG "YUVConvert"

YUVConvert::YUVConvert(int pixelWidth, int pixelHeight, PixelFormat pixelFormat) :
        pixel_width(pixelWidth),
        pixel_height(pixelHeight),
        pixel_format(pixelFormat),
        out_buffer(NULL),
        handle(NULL),
        pixel_wh(0) {
    if (pixelFormat == PIXEL_FORMAT_NV12) {
        pixel_wh = pixel_width * pixel_height;
        out_buffer = tjAlloc(pixel_width * pixel_height * 4);
    } else if (pixelFormat == PIXEL_FORMAT_YUV422) {
        subSample = TJSAMP_422;
        handle = tjInitDecompress();
        out_buffer = tjAlloc(pixel_width * pixel_height * tjPixelSize[TJPF_RGBA]);
    }
}

YUVConvert::~YUVConvert() {
    destroy();
}

void YUVConvert::destroy() {
    if (out_buffer) tjFree(out_buffer);
    if (handle) tjDestroy(handle);
    pixel_width = pixel_height = 0;
    pixel_format = PIXEL_FORMAT_ERROR;
}

unsigned char *YUVConvert::convertNV12(const uint8_t *data) {
    libyuv::NV12ToABGR(data, pixel_width, data + pixel_wh, pixel_width,
                       out_buffer, pixel_width * 4, pixel_width, pixel_height);
    return out_buffer;
}

//5~6ms
unsigned char *YUVConvert::convertYUV422(const uint8_t *data) {
    tjDecodeYUV(handle, (unsigned char *) data, 4, subSample, out_buffer,
                pixel_width, 0, pixel_height, TJPF_RGBA, flags);
    return out_buffer;
}





