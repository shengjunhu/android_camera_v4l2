//
// Created by Hsj on 2021/5/24.
//

#ifndef ANDROID_CAMERA_V4L2_DECODECREATOR_H
#define ANDROID_CAMERA_V4L2_DECODECREATOR_H

#include "IDecoder.h"

typedef enum DecodeType {
    DECODE_UNKNOWN = 0,
    DECODE_HW = 1,
    DECODE_SW = 2,
} decodeType;

class DecodeCreator {
private:
    DecodeType _type;
    IDecoder *decoder;
public:
    DecodeCreator();
    ~DecodeCreator();
    bool createType(DecodeType _type, int frameW, int frameH);
    DecodeType getDecodeType();
    bool start();
    uint8_t *convert(void *raw_buffer, unsigned int raw_size);
    bool stop();
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_DECODECREATOR_H
