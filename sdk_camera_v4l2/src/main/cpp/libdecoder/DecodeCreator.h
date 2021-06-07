//
// Created by Hsj on 2021/5/24.
//

#ifndef ANDROID_CAMERA_V4L2_DECODECREATOR_H
#define ANDROID_CAMERA_V4L2_DECODECREATOR_H

#include "IDecoder.h"

typedef enum {
    PIXEL_FORMAT_NV12   = 1, //yvu
    PIXEL_FORMAT_YUV422 = 2, //uyvy
    PIXEL_FORMAT_YUYV   = 3,
    PIXEL_FORMAT_GRAY16 = 4,
    PIXEL_FORMAT_DEPTH  = 5,
    PIXEL_FORMAT_ERROR  = 0,
} PixelFormat;

typedef enum DecodeType {
    DECODE_HW      = 1,
    DECODE_SW      = 2,
    DECODE_UNKNOWN = 0,
} decodeType;

class DecodeCreator {
private:
    DecodeType type;
    IDecoder *decoder;
public:
    DecodeCreator(int frameW, int frameH);
    ~DecodeCreator();
    PixelFormat getPixelFormat();
    bool start();
    uint8_t* convert(void *raw_buffer, unsigned long raw_size);
    bool stop();
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_DECODECREATOR_H
