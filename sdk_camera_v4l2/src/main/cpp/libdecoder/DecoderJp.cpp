//
// Created by Hsj on 2021/5/24.
//

#include <cstring>
#include <cstdlib>
#include "DecoderJp.h"

#define TAG "DecoderJp"

bool DecoderJp::create() {
    //1-Create decompress
    if (handle){
        tjDestroy(handle);
        handle = NULL;
    }
    handle = tjInitDecompress();
    //2-rgb memory
    SAFE_FREE(outBuffer)
    outBuffer = tjAlloc(width * height * tjPixelSize[TJPF_RGB]);
    return true;
}

bool DecoderJp::start() {
    LOGD(TAG, "start success")
    return true;
}

uint8_t *DecoderJp::convert(void *raw_buffer, unsigned int raw_size) {
    auto *raw = (unsigned char *) raw_buffer;
    //3-get raw_buffer info
    tjDecompressHeader3(handle, raw, raw_size, &_width, &_height, &_subSample, &_colorSpace);
    //4-decompress: 22ms
    tjDecompress2(handle, raw, raw_size, outBuffer,_width, 0, _height, TJPF_RGB, _flags);
    //5-tjInitTransform: rotation or flip
    //6-return
    return outBuffer;
}

bool DecoderJp::stop() {
    LOGD(TAG, "stop success")
    return true;
}

void DecoderJp::destroy() {
    //7-destroy handle
    if (handle){
        tjDestroy(handle);
        handle = NULL;
    }
    SAFE_FREE(outBuffer)
    width = 0;
    height = 0;
    _flags = 0;
    _width = 0;
    _height= 0;
    _subSample = 0;
    _colorSpace = 0;
    LOGD(TAG, "destroy success")
}