//
// Created by Hsj on 2021/6/9.
//

#ifndef ANDROID_CAMERA_V4L2_DECODERFACTORY_H
#define ANDROID_CAMERA_V4L2_DECODERFACTORY_H

#include "Common.h"

typedef enum {
    PIXEL_FORMAT_NV12   = 1, //yvu
    PIXEL_FORMAT_YUV422 = 2, //yuv
    PIXEL_FORMAT_YUYV   = 3, //yuyv
    PIXEL_FORMAT_DEPTH  = 4, //uint16
    PIXEL_FORMAT_ERROR  = 0,
} PixelFormat;

typedef enum DecodeType {
    DECODE_UNKNOWN = 0,
    DECODE_HW      = 1,
    DECODE_SW      = 2,
} decodeType;

class IDecoder {
protected:
    int width, height;
public:
    virtual ~IDecoder() = default;
    virtual bool create(int width, int height) = 0;
    virtual bool start() = 0;
    virtual uint8_t* convert2YUV(void *raw_buffer, unsigned long raw_size) = 0;
    virtual bool stop() = 0;
    virtual void destroy() = 0;
};

class DecoderFactory {
private:
    DecodeType type;
    IDecoder *decoder;
    volatile int action;
    inline const int onAction() const;
public:
    DecoderFactory(int frameW, int frameH);
    ~DecoderFactory();
    PixelFormat getPixelFormat();
    bool start();
    uint8_t* convert2YUV(void *raw_buffer, unsigned long raw_size);
    bool stop();
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_DECODERFACTORY_H
