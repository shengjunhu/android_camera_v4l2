//
// Created by Hsj on 2021/5/8.
//

#include "Common.h"
#include "HwDecoder.h"
#include <string>

#define MIME_TYPE "video/mjpeg"
#define TAG "HwDecoder"
#define TIME_OUT_US 1000
#define STATUS_CREATE 0
#define STATUS_PARAM  1
#define STATUS_INIT   2
#define STATUS_RUN    3

HwDecoder::HwDecoder() :
        width(0),
        height(0),
        mediaCodec(NULL),
        status(STATUS_CREATE) {
}

HwDecoder::~HwDecoder() {
    destroy();
}

inline const unsigned int HwDecoder::onStatus() const { return status; }

bool HwDecoder::updateSize(unsigned int frameW, unsigned int frameH) {
    if (STATUS_RUN != onStatus()) {
        width = frameW;
        height = frameH;
        status = STATUS_PARAM;
        return true;
    } else {
        LOGE(TAG, "updateSize failed: %d", onStatus())
        return false;
    }
}

bool HwDecoder::init() {
    bool ret = false;
    if (STATUS_PARAM > onStatus()) {
        LOGE(TAG, "init failed: %d", onStatus())
    } else if (width == 0 || height == 0) {
        LOGE(TAG, "init frameWidth or frameHeight is error")
    } else {
        //1.1-创建MediaCodec
        if (mediaCodec == NULL) {
            mediaCodec = AMediaCodec_createDecoderByType(MIME_TYPE);
        }
        if (mediaCodec == NULL) {
            LOGE(TAG, "Not support for %s", MIME_TYPE)
        } else {
            //1.2-配置MediaCodec参数
            AMediaFormat *mediaFormat = AMediaFormat_new();
            AMediaFormat_setString(mediaFormat, "mime", MIME_TYPE);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, width);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, height);
            if (AMEDIA_OK == AMediaCodec_configure(mediaCodec, mediaFormat, NULL, NULL, 0)) {
                LOGD(TAG, "create success")
                AMediaFormat_delete(mediaFormat);
                status = STATUS_INIT;
                ret = true;
            } else {
                LOGE(TAG, "configuration failure")
                AMediaFormat_delete(mediaFormat);
                AMediaCodec_delete(mediaCodec);
                mediaCodec = NULL;
            }
        }
    }
    return ret;
}

bool HwDecoder::start() {
    //2-启动MediaCodec
    bool ret = false;
    if (STATUS_INIT == onStatus()) {
        if (AMEDIA_OK == AMediaCodec_start(mediaCodec)) {
            LOGD(TAG, "start success")
            status = STATUS_RUN;
            ret = true;
        } else {
            LOGE(TAG, "start failure")
        }
    } else {
        LOGW(TAG, "Please call init first after call start")
    }
    return ret;
}

uint8_t *HwDecoder::process(void *raw_buffer, size_t raw_size) {
    uint8_t *out_buffer = NULL;
    if (STATUS_RUN != onStatus()) return out_buffer;
    //3.1-获取可用的输入缓冲区的索引
    ssize_t in_buffer_id = AMediaCodec_dequeueInputBuffer(mediaCodec, TIME_OUT_US);
    if (in_buffer_id < 0) {
        LOGW(TAG, "No available input buffer")
        //AMediaCodec_flush(mediaCodec);
    } else {
        //3.2-获取可用的输入缓冲区
        size_t out_size = 0;
        uint8_t *in_buffer = AMediaCodec_getInputBuffer(mediaCodec, in_buffer_id, &out_size);
        //3.3-将填满数据的inputBuffer提交到解码队列
        memcpy(in_buffer, raw_buffer, raw_size);
        AMediaCodec_queueInputBuffer(mediaCodec, in_buffer_id, 0, raw_size, timeUs(), 0);
        //3.4-获取已成功编解码的输出缓冲区的索引
        AMediaCodecBufferInfo info;
        ssize_t out_buffer_id = AMediaCodec_dequeueOutputBuffer(mediaCodec, &info, TIME_OUT_US);
        if (out_buffer_id >= 0) {
            //3.5-获取输出缓冲区
            out_buffer = AMediaCodec_getOutputBuffer(mediaCodec, out_buffer_id, NULL);
            //3.6-释放输出缓冲区
            AMediaCodec_releaseOutputBuffer(mediaCodec, out_buffer_id, false);
        } else if (out_buffer_id == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            LOGW(TAG, "Output buffers changed")
        } else if (out_buffer_id == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            AMediaFormat *format = AMediaCodec_getOutputFormat(mediaCodec);
            int _width = 0, _height = 0, _color = 0;
            if (format) {
                AMediaFormat_getInt32(format, "width", &_width);
                AMediaFormat_getInt32(format, "height", &_height);
                AMediaFormat_getInt32(format, "color-format", &_color);
                AMediaFormat_delete(format);
            }
            LOGW(TAG, "w=%d, h=%d, color=%d", _width, _height, _color)
        } else if (out_buffer_id == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            LOGW(TAG, "No output buffer right now")
        } else {
            LOGW(TAG, "Unexpected info code: %zd", out_buffer_id)
        }
    }
    //3.7-返回解码数据
    return out_buffer;
}

bool HwDecoder::stop() {
    //4-stop decode
    bool ret = false;
    if (STATUS_RUN == onStatus()) {
        status = STATUS_INIT;
        if (AMEDIA_OK == AMediaCodec_stop(mediaCodec)) {
            LOGD(TAG, "stop success")
            ret = true;
        } else {
            LOGE(TAG, "stop failure")
        }
    } else {
        LOGW(TAG, "Please call start first after call stop")
    }
    return ret;
}

void HwDecoder::destroy() {
    //5-destroy
    if (STATUS_RUN == onStatus()) {
        stop();
    }
    width = 0;
    height = 0;
    status = STATUS_CREATE;
    if (mediaCodec != NULL) {
        AMediaCodec_delete(mediaCodec);
        mediaCodec = NULL;
    }
    LOGD(TAG, "destroy success")
}