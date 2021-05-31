//
// Created by Hsj on 2021/5/24.
//

#include <cstring>
#include <cstdlib>
#include <libyuv.h>
#include "DecoderHw.h"

#define TAG "DecoderHw"
#define TIME_OUT_US 3000
#define MIME_TYPE "video/mjpeg"

bool DecoderHw::create() {
    if (mediaCodec == NULL) {
        mediaCodec = AMediaCodec_createDecoderByType(MIME_TYPE);
    }
    AMediaFormat *mediaFormat = AMediaFormat_new();
    AMediaFormat_setString(mediaFormat, AMEDIAFORMAT_KEY_MIME, MIME_TYPE);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_FRAME_RATE, 30);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, 21);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_BIT_RATE, width * height);
    if (AMEDIA_OK == AMediaCodec_configure(mediaCodec, mediaFormat, NULL, NULL, 0)) {
        //AMediaFormat_delete(format);
        SAFE_FREE(outBuffer)
        frameWH = width * height;
        outBuffer = (uint8_t *) calloc(1, frameWH * 3);
        LOGD(TAG, "create success")
        return true;
    } else {
        LOGE(TAG, "create failure")
        AMediaFormat_delete(mediaFormat);
        AMediaCodec_delete(mediaCodec);
        mediaCodec = NULL;
        return false;
    }
}

bool DecoderHw::start() {
    if (AMEDIA_OK == AMediaCodec_start(mediaCodec)) {
        LOGD(TAG, "start success")
        return true;
    } else {
        LOGE(TAG, "start failure")
        return false;
    }
}

uint8_t *DecoderHw::convertRGB(uint8_t *nv12) {
    if (nv12 != NULL) {
        libyuv::NV12ToRAW(nv12, width,nv12 + frameWH,width,
                outBuffer, width * 3,width, height);
        return outBuffer;
    } else {
        return NULL;
    }
}

uint8_t *DecoderHw::convert(void *raw_buffer, unsigned int raw_size) {
    uint8_t *out_buffer = NULL;
    //1-get input buffer index on buffers
    ssize_t in_buffer_id = AMediaCodec_dequeueInputBuffer(mediaCodec, TIME_OUT_US);
    if (in_buffer_id < 0) {
        //TODO 当获取输入index不可用时，可采用循环do{}while()获取到输入index为止
        LOGW(TAG, "No available input buffer")
    } else {
        //2-get input buffer by input buffer index
        size_t out_size;
        uint8_t *in_buffer = AMediaCodec_getInputBuffer(mediaCodec, in_buffer_id, &out_size);
        //3-put raw buffer to input buffer
        memcpy(in_buffer, raw_buffer, raw_size);
        //4-submit input buffer to queue buffers of input
        AMediaCodec_queueInputBuffer(mediaCodec, in_buffer_id, 0, raw_size, timeUs(), 0);
        //TODO 当前解码是队列方式,如果取当前帧解码的数据要加do{}while(),直到输出返回,耗时30ms左右
        //5-get out buffer index of decode by output queue buffers
        AMediaCodecBufferInfo info;
        ssize_t out_buffer_id = AMediaCodec_dequeueOutputBuffer(mediaCodec, &info, TIME_OUT_US);
        if (out_buffer_id >= 0) {
            //6-get output buffer by output buffer index
            out_buffer = AMediaCodec_getOutputBuffer(mediaCodec, out_buffer_id, &out_size);
            //7-release output buffer by output buffer index
            AMediaCodec_releaseOutputBuffer(mediaCodec, out_buffer_id, false);
        } else if (out_buffer_id == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            LOGW(TAG, "media info output buffers changed")
        } else if (out_buffer_id == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            LOGW(TAG, "media info output format changed")
            /*AMediaFormat *format = AMediaCodec_getOutputFormat(mediaCodec);
            int _width = 0, _height = 0, _color = 0;
            if (format) {
                AMediaFormat_getInt32(format, "width", &_width);
                AMediaFormat_getInt32(format, "height", &_height);
                AMediaFormat_getInt32(format, "color-format", &_color);
                //AMediaFormat_delete(format);
                LOGD(TAG, "width=%d,height=%d,format=%d", _width, _height, _color)
            }*/
        } else if (out_buffer_id == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            LOGW(TAG, "media info try again later")
        } else {
            LOGW(TAG, "Unexpected info code: %zd", out_buffer_id)
        }
    }
    //8-return
    return convertRGB(out_buffer);
}

bool DecoderHw::stop() {
    if (AMEDIA_OK == AMediaCodec_stop(mediaCodec)) {
        LOGD(TAG, "stop success")
        return true;
    } else {
        LOGE(TAG, "stop failure")
        return false;
    }
}

void DecoderHw::destroy() {
    if (mediaCodec != NULL) {
        AMediaCodec_delete(mediaCodec);
        mediaCodec = NULL;
    }
    SAFE_FREE(outBuffer)
    width = 0;
    height = 0;
    frameWH = 0;
    LOGD(TAG, "destroy success")
}