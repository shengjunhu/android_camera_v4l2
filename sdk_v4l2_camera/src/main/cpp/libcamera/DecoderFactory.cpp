//
// Created by Hsj on 2021/6/9.
//

#include <cstring>
#include <cstdlib>
#include <exception>
#include "DecoderFactory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "DecoderFactory"

//======================================DecoderHw.cpp=============================================//

#include <media/NdkMediaCodec.h>
#define MIME_TYPE "video/mjpeg"
#define TIME_OUT_US 3000

class DecoderHw : public IDecoder {
private:
    AMediaCodec* mediaCodec;
public:
    DecoderHw():mediaCodec(NULL){}

    ~DecoderHw() override {
        if (mediaCodec) {
            AMediaCodec_stop(mediaCodec);
            AMediaCodec_delete(mediaCodec);
            mediaCodec = NULL;
        }
    }

    int init(uint16_t width, uint16_t height) override {
        int ret = -2;
        if (mediaCodec) {
            AMediaCodec_delete(mediaCodec);
            mediaCodec = NULL;
        }
        //1 create MediaCodec
        mediaCodec = AMediaCodec_createDecoderByType(MIME_TYPE);
        if (mediaCodec) {
            AMediaFormat* mediaFormat = AMediaFormat_new();
            AMediaFormat_setString(mediaFormat, AMEDIAFORMAT_KEY_MIME, MIME_TYPE);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, width);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, height);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_FRAME_RATE, 30);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, 21);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1);
            AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_BIT_RATE, width * height);
            if (AMEDIA_OK == AMediaCodec_configure(mediaCodec, mediaFormat, NULL, NULL, 0)) {
                ret = AMediaCodec_start(mediaCodec);
            } else {
                AMediaCodec_delete(mediaCodec);
                mediaCodec = NULL;
                ret = -1;
            }
            AMediaFormat_delete(mediaFormat);
        }
        return ret;
    }

    //3ms
    uint8_t* convert2YUV(void* raw_buffer, size_t raw_size) override {
        size_t out_size;
        //3.1 get input buffer index on buffers
        ssize_t in_buffer_id = AMediaCodec_dequeueInputBuffer(mediaCodec, TIME_OUT_US);
        if (in_buffer_id >= 0) {
            //3.2 get input buffer by input buffer index
            uint8_t *in_buffer = AMediaCodec_getInputBuffer(mediaCodec, in_buffer_id, &out_size);
            //3.3 put raw buffer to input buffer
            memcpy(in_buffer, raw_buffer, raw_size);
            //3.4 submit input buffer to queue buffers of input
            AMediaCodec_queueInputBuffer(mediaCodec, in_buffer_id, 0, raw_size, timeUs(), 0);
        } else {
            LOGW(TAG, "Hardware: No available input buffer");
        }

        //3.5 get out buffer index of decode by output queue buffers
        uint8_t* out = NULL;
        AMediaCodecBufferInfo info;
        ssize_t out_buffer_id = AMediaCodec_dequeueOutputBuffer(mediaCodec, &info, 0);
        if (out_buffer_id >= 0) {
            //3.6 get output buffer by output buffer index, nv12
            out = AMediaCodec_getOutputBuffer(mediaCodec, out_buffer_id, &out_size);
            //3.7 release output buffer by output buffer index
            AMediaCodec_releaseOutputBuffer(mediaCodec, out_buffer_id, info.size != 0);
        } else if (out_buffer_id == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            LOGW(TAG, "Hardware: media info output buffers changed");
        } else if (out_buffer_id == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            LOGW(TAG, "Hardware: media info output format changed");
            AMediaFormat *format = AMediaCodec_getOutputFormat(mediaCodec);
            LOGD(TAG, "AMediaFormat: %s", AMediaFormat_toString(format));
            AMediaFormat_delete(format);
        } else if (out_buffer_id == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            LOGW(TAG, "Hardware: media info try again later");
        } else {
            LOGW(TAG, "Hardware: Unexpected info code: %zd", out_buffer_id);
        }
        //3.8 return nv12
        return out;
    }

};

//*****************************************DecoderSw.cpp******************************************//

#include <turbojpeg.h>

class DecoderSw : public IDecoder {
private:
    int flags = 0;
    int _width = 0;
    int _height = 0;
    int subSample = 0;
    int colorSpace = 0;
    tjhandle handle;
    uint8_t* out_buffer;
public:
    DecoderSw():handle(NULL), out_buffer(NULL){}

    ~DecoderSw() override {
        //6 destroy handle
        if (out_buffer) {
            tjFree(out_buffer);
            out_buffer = nullptr;
        }
        if (handle) {
            tjDestroy(handle);
            handle = nullptr;
        }
    }

    int init(uint16_t width, uint16_t height) override {
        //1 create decompress
        handle = tjInitDecompress();
        //2 alloc yuv422 out buffer memory: subSample = TJSAMP_422
        size_t out_buffer_size = tjBufSizeYUV2(width, 4, height, TJSAMP_422);
        out_buffer = tjAlloc(out_buffer_size);
        LOGD(TAG, "DecoderSw: create success");
        return 0;
    }

    //20ms
    uint8_t* convert2YUV(void* raw_buffer, size_t raw_size) override {
        auto *raw = (unsigned char *) raw_buffer;
        //4 get raw_buffer info: subSample = TJSAMP_422
        tjDecompressHeader3(handle, raw, raw_size, &_width, &_height, &subSample, &colorSpace);
        //5 decompress: to YUV422 22ms (flag = 0、TJFLAG_FASTDCT)
        tjDecompressToYUV2(handle, raw, raw_size, out_buffer, _width, 4, _height, flags);
        return out_buffer;
    }

};

//*****************************************DecoderFactory.cpp*************************************//

DecoderFactory::DecoderFactory():decoder(NULL) {

}

DecoderFactory::~DecoderFactory() {
    SAFE_DELETE(decoder);
}

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

int DecoderFactory::init(uint16_t frameW, uint16_t frameH) {
    int ret = 0;
    SAFE_DELETE(decoder);
    type = DECODE_UNKNOWN;
    if (frameW <= 0 || frameH <= 0) {
        ret = -9;
        LOGE(TAG, "init frameW or frameH is error");
    } else {
        decoder = new DecoderHw();
        if (0 == decoder->init(frameW, frameH)) {
            type = DECODE_HW;
            LOGD(TAG, "decode by Hardware");
        } else {
            SAFE_DELETE(decoder)
            decoder = new DecoderSw();
            ret = decoder->init(frameW, frameH);
            type = DECODE_SW;
            LOGD(TAG, "decode by Software");
        }
    }
    return ret;
}

uint8_t* DecoderFactory::convert2YUV(void *raw_buffer, size_t raw_size) {
    if (LIKELY(decoder)) {
        return decoder->convert2YUV(raw_buffer, raw_size);
    } else {
        LOGW(TAG, "convert2YUV: decoder not init");
        return NULL;
    }
}

#ifdef __cplusplus
}  // extern "C"
#endif