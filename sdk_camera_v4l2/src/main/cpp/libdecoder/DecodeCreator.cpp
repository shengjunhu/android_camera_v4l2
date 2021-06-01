//
// Created by Hsj on 2021/5/24.
//

#include "DecoderHw.h"
#include "DecoderJp.h"
#include "DecodeCreator.h"

#define TAG "DecodeCreator"

DecodeCreator::DecodeCreator() :
        _type(DECODE_UNKNOWN),
        decoder(NULL){
}

DecodeCreator::~DecodeCreator() {
    destroy();
}

bool DecodeCreator::createType(DecodeType type, int frameW, int frameH) {
    bool ret = false;
    if (decoder) {
        SAFE_DELETE(decoder)
    } else if (frameW <= 0 || frameH <= 0) {
        LOGE(TAG, "init frameW or frameH is error")
    } else if (DECODE_HW == type) {
        _type = type;
        decoder = CREATE_CLASS(DecoderHw)
    } else if (DECODE_SW == type){
        _type = type;
        decoder = CREATE_CLASS(DecoderJp)
    }
    if (decoder) {
        decoder->width = frameW;
        decoder->height = frameH;
        ret = decoder->create();
        if (!ret) {
            SAFE_DELETE(decoder)
            _type = DECODE_UNKNOWN;
        }
    }
    return ret;
}

DecodeType DecodeCreator::getDecodeType(){
    return _type;
}

bool DecodeCreator::start() {
    if (decoder) {
        return decoder->start();
    } else {
        return false;
    }
}

uint8_t *DecodeCreator::convert(void *raw_buffer, unsigned int raw_size) {
    if (decoder) {
        return decoder->convert(raw_buffer, raw_size);
    } else {
        return NULL;
    }
}

bool DecodeCreator::stop() {
    if (decoder) {
        return decoder->stop();
    } else {
        return false;
    }
}

void DecodeCreator::destroy() {
    if (decoder) {
        decoder->destroy();
        SAFE_DELETE(decoder)
    }
}