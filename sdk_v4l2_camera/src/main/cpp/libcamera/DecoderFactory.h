//
// Created by Hsj on 2021/6/9.
//

#ifndef ANDROID_CAMERA_V4L2_DECODERFACTORY_H
#define ANDROID_CAMERA_V4L2_DECODERFACTORY_H

#include "Common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DecodeType {
    DECODE_UNKNOWN = 0,
    DECODE_HW      = 1,
    DECODE_SW      = 2,
} DecodeTypeEnum;

typedef enum PixelFormat {
    PIXEL_FORMAT_NV12   = 1, //yvu
    PIXEL_FORMAT_YUV422 = 2, //yuv
    PIXEL_FORMAT_YUYV   = 3, //yuyv
    PIXEL_FORMAT_DEPTH  = 4, //uint16
    PIXEL_FORMAT_ERROR  = 0,
} PixelFormatEnum;

class IDecoder {
protected:
    int width, height;
public:
    virtual ~IDecoder() = default;
    virtual int init(uint16_t width, uint16_t height) = 0;
    virtual uint8_t* convert2YUV(void* raw_buffer, size_t raw_size) = 0;
};

class DecoderFactory {
private:
    DecodeType type;
    IDecoder* decoder;
public:
    DecoderFactory();
    ~DecoderFactory();
    int init(uint16_t frameW, uint16_t frameH);
    PixelFormat getPixelFormat();
    uint8_t* convert2YUV(void* raw_buffer, size_t raw_size);
};

#ifdef __cplusplus
}  // extern "C"
#endif

#endif //ANDROID_CAMERA_V4L2_DECODERFACTORY_H
