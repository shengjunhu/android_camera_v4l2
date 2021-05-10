//
// Created by Hsj on 2021/5/8.
//

#include "Common.h"
#include "HwDecoder.h"
#include <ctime>
#include <string>
#include <android/native_window.h>

#define MIME_TYPE "video/mjpeg"
#define TAG "HwDecoder"
#define TIME_OUT_US 1000
#define STATUS_CREATE 0
#define STATUS_PARAM  1
#define STATUS_INIT   2
#define STATUS_RUN    3

long long timeUs() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (long long)time.tv_sec * 1000000 + time.tv_usec;
}

HwDecoder::HwDecoder() :
        frameWidth(0),
        frameHeight(0),
        surface(NULL),
        mediaCodec(NULL),
        status(STATUS_CREATE){
}

HwDecoder::~HwDecoder() {
    destroy();
}

inline const unsigned int HwDecoder::onStatus() const { return status; }

bool HwDecoder::updateSize(unsigned int width, unsigned int height) {
    if (STATUS_RUN != onStatus()){
        frameWidth = width;
        frameHeight = height;
        status = STATUS_PARAM;
        return true;
    } else {
        LOGW(TAG, "updateSize failed: %d",onStatus())
        return false;
    }
}

bool HwDecoder::setPreview(ANativeWindow *window) {
    if (STATUS_RUN != onStatus()){
        surface = window;
        return true;
    } else {
        LOGW(TAG, "setPreview failed: %d",onStatus())
        return false;
    }
}

bool HwDecoder::init() {
    if (STATUS_PARAM > onStatus()) {
        LOGW(TAG, "init failed: %d",onStatus())
        return false;
    } else if (frameWidth == 0 || frameHeight == 0){
        LOGW(TAG, "init frameWidth or frameHeight is error")
        return false;
    } else {
        //1.1-创建MediaCodec
        if(mediaCodec == NULL){
            mediaCodec = AMediaCodec_createDecoderByType(MIME_TYPE);
        }
        if (mediaCodec == NULL) {
            LOGE(TAG, "Not support for %s", MIME_TYPE)
            return false;
        } else {
            //1.2-配置MediaCodec参数
            AMediaFormat* mediaFormat = AMediaFormat_new();
            AMediaFormat_setString(mediaFormat, "mime", MIME_TYPE);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, frameWidth);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, frameHeight);
            //AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT,0x7F000789);
            //AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT,ANativeWindow_getFormat(surface));
            if (AMEDIA_OK == AMediaCodec_configure(mediaCodec, mediaFormat, NULL, NULL, 0)) {
                LOGD(TAG, "create success")
                //AMediaCodec_setOutputSurface(mediaCodec, surface);
                AMediaFormat_delete(mediaFormat);
                status = STATUS_INIT;
                return true;
            } else {
                LOGE(TAG, "configuration failure")
                AMediaFormat_delete(mediaFormat);
                AMediaCodec_delete(mediaCodec);
                mediaCodec = NULL;
                return false;
            }
        }
    }
}

bool HwDecoder::start() {
    //2-启动MediaCodec
    if (STATUS_INIT == onStatus()){
        if (AMEDIA_OK == AMediaCodec_start(mediaCodec)) {
            LOGD(TAG, "start success")
            status = STATUS_RUN;
            return true;
        } else {
            LOGE(TAG, "start failure")
            return false;
        }
    } else {
        LOGW(TAG, "Please call init first after call start")
        return false;
    }
}

uint8_t* HwDecoder::process(void *raw_buffer, size_t raw_size) {
    uint8_t *out_buffer = NULL;
    if (STATUS_RUN != onStatus()) return out_buffer;
    //3.1-获取可用的输入缓冲区的索引
    ssize_t in_buffer_id = AMediaCodec_dequeueInputBuffer(mediaCodec, TIME_OUT_US);
    if (in_buffer_id < 0){
        LOGW(TAG,"No available input buffer")
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
        if (out_buffer_id >= 0){
            //3.5-获取输出缓冲区
            out_buffer = AMediaCodec_getOutputBuffer(mediaCodec, out_buffer_id, NULL);
            //3.6-释放输出缓冲区
            AMediaCodec_releaseOutputBuffer(mediaCodec, out_buffer_id, false/*(surface != NULL && info.size != 0)*/);
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
            LOGW(TAG,"w=%d, h=%d, color=%d", _width, _height, _color)
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
    if (STATUS_RUN == onStatus()){
        status = STATUS_INIT;
        //AMediaCodec_flush(mediaCodec);
        if (AMEDIA_OK == AMediaCodec_stop(mediaCodec)) {
            LOGD(TAG, "stop success")
            return true;
        } else {
            LOGE(TAG, "stop failure")
            return false;
        }
    } else {
        LOGW(TAG, "Please call start first after call stop")
        return false;
    }
}

void HwDecoder::destroy() {
    //5-destroy
    if (STATUS_RUN == onStatus()){
        stop();
    }
    frameWidth = 0;
    frameHeight = 0;
    status = STATUS_CREATE;
    if (mediaCodec) {
        AMediaCodec_delete(mediaCodec);
        mediaCodec = NULL;
    }
    LOGD(TAG, "destroy success")
}