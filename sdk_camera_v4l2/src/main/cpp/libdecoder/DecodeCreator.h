//
// Created by Hsj on 2021/5/24.
//

#ifndef ANDROID_CAMERA_V4L2_DECODECREATOR_H
#define ANDROID_CAMERA_V4L2_DECODECREATOR_H

#include "IDecoder.h"

typedef enum DecodeType {
    DECODE_HW = 0,
    DECODE_SW = 1,
} decodeType;

class DecodeCreator {
private:
    IDecoder *decoder;
public:
    DecodeCreator();
    ~DecodeCreator();
    bool createType(DecodeType type, int frameW, int frameH);
    bool start();
    uint8_t *convert(void *raw_buffer, unsigned int raw_size);
    bool stop();
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_DECODECREATOR_H
