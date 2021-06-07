//
// Created by Hsj on 2021/5/24.
//

#include "DecoderHw.h"
#include "DecoderSw.h"
#include "DecodeCreator.h"

#define TAG "DecodeCreator"

DecodeCreator::DecodeCreator(int frameW, int frameH) : decoder(NULL) {
    if (frameW <= 0 || frameH <= 0) {
        LOGE(TAG, "init frameW or frameH is error")
    } else {
        decoder = CREATE_CLASS(DecoderHw);
        decoder->width = frameW;
        decoder->height = frameH;
        bool ret = decoder->create();
        if (ret) {
            type = DECODE_HW;
            LOGD(TAG, "decode by Hardware")
        } else {
            decoder->destroy();
            SAFE_DELETE(decoder)
            type = DECODE_SW;
            decoder = CREATE_CLASS(DecoderSw);
            decoder->width = frameW;
            decoder->height = frameH;
            decoder->create();
            LOGD(TAG, "decode by Software")
        }
    }
}

DecodeCreator::~DecodeCreator() {
    destroy();
}

PixelFormat DecodeCreator::getPixelFormat() {
    switch (type){
        case DECODE_HW:
            return PIXEL_FORMAT_NV12;
        case DECODE_SW:
            return PIXEL_FORMAT_YUV422;
        case DECODE_UNKNOWN:
        default:
            return PIXEL_FORMAT_ERROR;
    }
}

bool DecodeCreator::start() {
    if (LIKELY(decoder)) {
        return decoder->start();
    } else {
        LOGE(TAG, "decoder is NULL")
        return false;
    }
}

uint8_t* DecodeCreator::convert(void *raw_buffer, unsigned long raw_size) {
    if (LIKELY(decoder)) {
        return decoder->convert(raw_buffer, raw_size);
    } else {
        LOGE(TAG, "decoder is NULL")
        return NULL;
    }
}

bool DecodeCreator::stop() {
    if (LIKELY(decoder)) {
        return decoder->stop();
    } else {
        LOGE(TAG, "decoder is NULL")
        return false;
    }
}

void DecodeCreator::destroy() {
    if (LIKELY(decoder)) {
        decoder->destroy();
        SAFE_DELETE(decoder)
    }
}