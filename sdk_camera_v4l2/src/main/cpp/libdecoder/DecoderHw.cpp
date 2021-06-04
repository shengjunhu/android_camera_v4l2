//
// Created by Hsj on 2021/5/24.
//

#include <cstring>
#include <cstdlib>
#include "DecoderHw.h"

#define TAG "DecoderHw"
#define TIME_OUT_US 3000
#define MIME_TYPE "video/mjpeg"

bool DecoderHw::create() {
    //1-Create MediaCodec
    mediaCodec = AMediaCodec_createDecoderByType(MIME_TYPE);
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
        SAFE_FREE(out_buffer)
        frameWH = width * height;
        out_buffer = (uint8_t *) calloc(1, frameWH * 3);
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
    //2-Start
    if (AMEDIA_OK == AMediaCodec_start(mediaCodec)) {
        LOGD(TAG, "start success")
        return true;
    } else {
        LOGE(TAG, "start failure")
        return false;
    }
}

uint8_t *DecoderHw::convert(void *raw_buffer, unsigned long raw_size) {
    uint8_t *out_buffer = NULL;
    //3.1-get input buffer index on buffers
    ssize_t in_buffer_id = AMediaCodec_dequeueInputBuffer(mediaCodec, TIME_OUT_US);
    if (in_buffer_id < 0) {
        //当获取输入index不可用时，可采用循环do{}while()获取到输入index为止
        LOGW(TAG, "No available input buffer")
    } else {
        //3.2-get input buffer by input buffer index
        size_t out_size;
        uint8_t *in_buffer = AMediaCodec_getInputBuffer(mediaCodec, in_buffer_id, &out_size);
        //3.3-put raw buffer to input buffer
        memcpy(in_buffer, raw_buffer, raw_size);
        //3.4-submit input buffer to queue buffers of input
        AMediaCodec_queueInputBuffer(mediaCodec, in_buffer_id, 0, raw_size, timeUs(), 0);
        //当前解码是队列方式,如果取当前帧解码的数据要加do{}while(),直到输出返回,耗时30ms左右
        //3.5-get out buffer index of decode by output queue buffers
        AMediaCodecBufferInfo info;
        ssize_t out_buffer_id = AMediaCodec_dequeueOutputBuffer(mediaCodec, &info, TIME_OUT_US);
        if (out_buffer_id >= 0) {
            //3.6-get output buffer by output buffer index
            out_buffer = AMediaCodec_getOutputBuffer(mediaCodec, out_buffer_id, &out_size);
            //3.7-release output buffer by output buffer index
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
    return out_buffer;
}

bool DecoderHw::stop() {
    //4-Stop
    if (AMEDIA_OK == AMediaCodec_stop(mediaCodec)) {
        LOGD(TAG, "stop success")
        return true;
    } else {
        LOGE(TAG, "stop failure")
        return false;
    }
}

void DecoderHw::destroy() {
    //5-Destroy
    if (mediaCodec != NULL) {
        AMediaCodec_delete(mediaCodec);
        mediaCodec = NULL;
    }
    SAFE_FREE(out_buffer)
    width = 0;
    height = 0;
    frameWH = 0;
    LOGD(TAG, "destroy success")
}