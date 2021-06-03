//
// Created by Hsj on 2021/5/24.
//

#include <cstring>
#include <cstdlib>
#include "DecoderJp.h"

#define TAG "DecoderJp"

bool DecoderJp::create() {
    //1-Create decompress
    handle = tjInitDecompress();
    //2-Alloc out buffer memory
    out_buffer = tjAlloc(width * height * tjPixelSize[TJPF_RGB]);
    return true;
}

bool DecoderJp::start() {
    LOGD(TAG, "start success")
    return true;
}

uint8_t *DecoderJp::convert(void *raw_buffer, unsigned long raw_size) {
    auto *raw = (unsigned char *) raw_buffer;
    //3-Get raw_buffer info
    tjDecompressHeader3(handle, raw, raw_size, &_width, &_height, &_subSample, &_colorSpace);

    //4-Decompress: 22ms
    tjDecompress2(handle, raw, raw_size, out_buffer,_width, 0, _height, TJPF_RGB, _flags);

    //5-Rotation or flip by tjInitTransform

    //6-Return
    return out_buffer;
}

bool DecoderJp::stop() {
    LOGD(TAG, "stop success")
    return true;
}

void DecoderJp::destroy() {
    //7-Destroy handle
    if (handle){
        tjDestroy(handle);
        handle = NULL;
    }
    SAFE_FREE(out_buffer)
    width = 0;
    height = 0;
    _flags = 0;
    _width = 0;
    _height= 0;
    _subSample = 0;
    _colorSpace = 0;
    LOGD(TAG, "destroy success")
}