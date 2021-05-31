//
// Created by Hsj on 2021/5/24.
//

#ifndef ANDROID_CAMERA_V4L2_IDECODER_H
#define ANDROID_CAMERA_V4L2_IDECODER_H

#include "Common.h"

#define CREATE_CLASS(classType) \
new classType();

#define CONSTRUCT_CLASS(classType)\
public: classType() {}\
public: ~classType() {}

class IDecoder {
private:
    inline const unsigned int onStatus() const;
    volatile int status = 0;
public:
    int width,height;
    CONSTRUCT_CLASS(IDecoder);
    virtual bool create();
    virtual bool start();
    virtual uint8_t* convert(void *raw_buffer, unsigned int raw_size);
    virtual bool stop();
    virtual void destroy();
};

#endif //ANDROID_CAMERA_V4L2_IDECODER_H
