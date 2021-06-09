//
// Created by Hsj on 2021/6/9.
//

#include <cstring>
#include <cstdlib>
#include "DecoderFactory.h"
#define TAG "DecoderFactory"

//*****************************************DecoderHw.cpp******************************************//

#include <libyuv.h>
#include <media/NdkMediaCodec.h>
#define MIME_TYPE "video/mjpeg"
#define TIME_OUT_US 3000

class DecoderHw: public IDecoder {
private:
    int YSize = 0;
    uint8_t *rgba_buffer = NULL;
    AMediaCodec *mediaCodec = NULL;
public:
    bool create(int width, int height) override {
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
            LOGD(TAG, "Hardware: create success")
            this->width = width;
            this->height = height;
            YSize = width * height;
            rgba_buffer = (uint8_t *)malloc(width * height * 4);
            return true;
        } else {
            LOGE(TAG, "Hardware: create failure")
            AMediaFormat_delete(mediaFormat);
            AMediaCodec_delete(mediaCodec);
            mediaCodec = NULL;
            return false;
        }
    }

    bool start() override {
        //2-Start
        if (AMEDIA_OK == AMediaCodec_start(mediaCodec)) {
            LOGD(TAG, "Hardware: start success")
            return true;
        } else {
            LOGE(TAG, "Hardware: start failure")
            return false;
        }
    }

    //3ms
    uint8_t* convert2YUV(void *raw_buffer, unsigned long raw_size) override {
        //3.1-get input buffer index on buffers
        ssize_t in_buffer_id = AMediaCodec_dequeueInputBuffer(mediaCodec, TIME_OUT_US);
        if (in_buffer_id < 0) {
            //当获取输入index不可用时，可采用循环do{}while()获取到输入index为止
            LOGW(TAG, "Hardware: No available input buffer")
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
                uint8_t* out = AMediaCodec_getOutputBuffer(mediaCodec, out_buffer_id, &out_size);
                //3.7-release output buffer by output buffer index
                AMediaCodec_releaseOutputBuffer(mediaCodec, out_buffer_id, false);
                //3.8-return nv12
                return out;
            } else if (out_buffer_id == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
                LOGW(TAG, "Hardware: media info output buffers changed")
            } else if (out_buffer_id == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                LOGW(TAG, "Hardware: media info output format changed")
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
                LOGW(TAG, "Hardware: media info try again later")
            } else {
                LOGW(TAG, "Hardware: Unexpected info code: %zd", out_buffer_id)
            }
        }
        return NULL;
    }

    //5~6ms: nv12->rgba
    uint8_t* convert2RGBA(uint8_t *src_buffer, unsigned long src_size) override {
        libyuv::NV12ToABGR(src_buffer, width, src_buffer + YSize, width,
                           rgba_buffer, width * 4, width, height);
        return rgba_buffer;
    }

    bool stop() override {
        //4-Stop
        if (AMEDIA_OK == AMediaCodec_stop(mediaCodec)) {
            LOGD(TAG, "Hardware: stop success")
            return true;
        } else {
            LOGE(TAG, "Hardware: stop failure")
            return false;
        }
    }

    void destroy() override {
        //5-Destroy
        if (mediaCodec) {
            AMediaCodec_delete(mediaCodec);
            mediaCodec = NULL;
        }
        SAFE_FREE(rgba_buffer)
        YSize = width = height = 0;
        LOGD(TAG, "Hardware： destroy success")
    }
};

//*****************************************DecoderSw.cpp******************************************//

#include <turbojpeg.h>

class DecoderSw: public IDecoder {
private:
    int flags = 0;
    int _width  = 0;
    int _height = 0;
    int subSample = 0;
    int colorSpace = 0;
    tjhandle handle = NULL;
    uint8_t *rgba_buffer = NULL;
    unsigned char *out_buffer = NULL;
public:
    bool create(int width, int height) override {
        this->width = width;
        this->height = height;
        //1-Create decompress
        handle = tjInitDecompress();
        //2-Alloc yuv422 out buffer memory: subSample = TJSAMP_422
        size_t out_buffer_size = tjBufSizeYUV2(width, 4, height, TJSAMP_422);
        out_buffer = tjAlloc(out_buffer_size);
        //3-Alloc rgba out buffer memory
        rgba_buffer = tjAlloc(width * height * tjPixelSize[TJPF_RGBA]);
        LOGD(TAG,"DecoderSw: create success")
        return true;
    }

    bool start() override {
        //4-Start
        LOGD(TAG, "DecoderSw: start success")
        return true;
    }

    //20ms
    uint8_t* convert2YUV(void *raw_buffer, unsigned long raw_size) override {
        auto *raw = (unsigned char *) raw_buffer;
        //5-Get raw_buffer info: subSample = TJSAMP_422
        tjDecompressHeader3(handle, raw, raw_size, &_width, &_height, &subSample, &colorSpace);
        //6-Decompress: to YUV422 22ms (flag = 0、TJFLAG_FASTDCT)
        tjDecompressToYUV2(handle, raw, raw_size, out_buffer, _width, 4, _height, flags);
        return (uint8_t *) out_buffer;
    }

    //5~6ms: yuv422->rgba
    uint8_t* convert2RGBA(uint8_t *src_buffer, unsigned long src_size) override {
        tjDecodeYUV(handle,  src_buffer, 4, subSample, rgba_buffer,
                    _width, 0, _height, TJPF_RGBA, flags);
        return rgba_buffer;
    }

    bool stop() override {
        //7-Stop
        LOGD(TAG, "DecoderSw: stop success")
        return true;
    }

    void destroy() override {
        //8-Destroy handle
        if (out_buffer) {
            tjFree(out_buffer);
            out_buffer = nullptr;
        }
        if (rgba_buffer) {
            tjFree(rgba_buffer);
            rgba_buffer = nullptr;
        }
        if (handle) {
            tjDestroy(handle);
            handle = nullptr;
        }
        width = height = 0;
        _width = _height = 0;
        flags = subSample = colorSpace = 0;
        LOGD(TAG, "DecoderSw: destroy success")
    }
};

//*****************************************DecoderFactory.cpp*************************************//

enum { ACTION_INIT = 0, ACTION_CREATE = 1, ACTION_RUN = 2};

DecoderFactory::DecoderFactory(int frameW, int frameH) :decoder(NULL),action(ACTION_INIT) {
    if (frameW <= 0 || frameH <= 0) {
        LOGE(TAG, "init frameW or frameH is error")
    } else {
        decoder = new DecoderHw();
        if (decoder->create(frameW,frameH)) {
            type = DECODE_HW;
            action = ACTION_CREATE;
            LOGD(TAG, "decode by Hardware")
        } else {
            decoder->destroy();
            SAFE_DELETE(decoder)
            decoder = new DecoderSw();
            decoder->create(frameW,frameH);
            type = DECODE_SW;
            action = ACTION_CREATE;
            LOGD(TAG, "decode by Software")
        }
    }
}

DecoderFactory::~DecoderFactory() {
    destroy();
}

inline const int DecoderFactory::onAction() const { return action; }

PixelFormat DecoderFactory::getPixelFormat() {
    switch (type) {
        case DECODE_HW:
            return PIXEL_FORMAT_NV12;
        case DECODE_SW:
            return PIXEL_FORMAT_YUV422;
        case DECODE_UNKNOWN:
        default:
            return PIXEL_FORMAT_ERROR;
    }
}

bool DecoderFactory::start() {
    bool ret = false;
    if (LIKELY(ACTION_CREATE == onAction())) {
        ret = decoder->start();
        if (ret) action = ACTION_RUN;
    }else {
        LOGE(TAG, "start error: %d",onAction())
    }
    LOGD(TAG, "start: %s", ret ? "success" : "failed")
    return ret;
}

uint8_t* DecoderFactory::convert2YUV(void *raw_buffer, unsigned long raw_size) {
    if (LIKELY(ACTION_RUN == onAction())) {
        return decoder->convert2YUV(raw_buffer, raw_size);
    } else {
        LOGW(TAG, "convert2YUV error: %d",onAction())
        return NULL;
    }
}

uint8_t* DecoderFactory::convert2RGBA(uint8_t *src_buffer, unsigned long src_size) {
    if (LIKELY(ACTION_RUN == onAction())) {
        if (LIKELY(src_buffer)){
            return decoder->convert2RGBA(src_buffer, src_size);
        } else {
            LOGW(TAG, "convert2RGBA src buffer is null")
        }
    } else {
        LOGW(TAG, "convert2RGBA error: %d",onAction())
    }
    return NULL;
}

bool DecoderFactory::stop() {
    bool ret = false;
    if (LIKELY(ACTION_RUN == onAction())) {
        action = ACTION_CREATE;
        ret = decoder->stop();
    } else {
        LOGE(TAG, "stop error: %d",onAction())
    }
    LOGD(TAG, "stop: %s", ret ? "success" : "failed")
    return ret;
}

void DecoderFactory::destroy() {
    if (LIKELY(decoder)) {
        action = ACTION_INIT;
        decoder->destroy();
        SAFE_DELETE(decoder)
    }
    LOGD(TAG, "destroy: success")
}
