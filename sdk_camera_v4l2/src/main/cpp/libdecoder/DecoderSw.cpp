//
// Created by Hsj on 2021/5/24.
//

#include <cstring>
#include <cstdlib>
#include "DecoderSw.h"

#define TAG "DecoderSw"

bool DecoderSw::create() {
    //1-Create decompress
    handle = tjInitDecompress();
    //2-Alloc out buffer memory: subSample = TJSAMP_422
    out_buffer = tjAlloc(tjBufSizeYUV2(width, 4, height, TJSAMP_422));
    return true;
}

bool DecoderSw::start() {
    //3-Start
    LOGD(TAG, "start success")
    return true;
}

uint8_t *DecoderSw::convert(void *raw_buffer, unsigned long raw_size) {
    auto *raw = (unsigned char *) raw_buffer;
    //4-Get raw_buffer info: subSample = TJSAMP_422
    tjDecompressHeader3(handle, raw, raw_size, &_width, &_height, &_subSample, &_colorSpace);
    //5-Decompress: to YUV422 22ms (flag = 0、TJFLAG_FASTDCT)
    tjDecompressToYUV2(handle, raw, raw_size, out_buffer, _width, 4, _height, _flags);
    return out_buffer;
}

bool DecoderSw::stop() {
    //6-Stop
    LOGD(TAG, "stop success")
    return true;
}

void DecoderSw::destroy() {
    //7-Destroy handle
    if (out_buffer) {
        tjFree(out_buffer);
        out_buffer = NULL;
    }
    if (handle) {
        tjDestroy(handle);
        handle = NULL;
    }
    width = 0;
    height = 0;
    _flags = 0;
    _width = 0;
    _height = 0;
    _subSample = 0;
    _colorSpace = 0;
    LOGD(TAG, "destroy success")
}