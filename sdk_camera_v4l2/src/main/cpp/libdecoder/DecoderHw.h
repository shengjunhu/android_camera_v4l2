//
// Created by Hsj on 2021/5/24.
//

#ifndef ANDROID_CAMERA_V4L2_DECODERHW_H
#define ANDROID_CAMERA_V4L2_DECODERHW_H

#include "IDecoder.h"
#include <media/NdkMediaCodec.h>

class DecoderHw: public IDecoder {
private:
    AMediaCodec* mediaCodec = NULL;
public:
    CONSTRUCT_CLASS(DecoderHw);
    virtual bool create();
    virtual bool start();
    virtual uint8_t* convert(void *raw_buffer, unsigned long raw_size);
    virtual bool stop();
    virtual void destroy();
};

#endif //ANDROID_CAMERA_V4L2_DECODERHW_H
