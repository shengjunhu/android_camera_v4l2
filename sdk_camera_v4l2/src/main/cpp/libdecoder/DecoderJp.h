//
// Created by Hsj on 2021/5/24.
//

#ifndef ANDROID_CAMERA_V4L2_DECODERJP_H
#define ANDROID_CAMERA_V4L2_DECODERJP_H

#include <turbojpeg.h>
#include "IDecoder.h"

class DecoderJp : public IDecoder {
private:
    int _flags;
    int _width;
    int _height;
    int _subSample;
    int _colorSpace;
    tjhandle handle = NULL;
    unsigned char *out_buffer = NULL;
public:
    CONSTRUCT_CLASS(DecoderJp);
    virtual bool create();
    virtual bool start();
    virtual uint8_t *convert(void *raw_buffer, unsigned long raw_size);
    virtual bool stop();
    virtual void destroy();
};

#endif //ANDROID_CAMERA_V4L2_DECODERJP_H
