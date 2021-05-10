//
// Created by Hsj on 2021/5/8.
//

#ifndef ANDROID_CAMERA_V4L2_HWDECODER_H
#define ANDROID_CAMERA_V4L2_HWDECODER_H

#include <media/NdkMediaCodec.h>

class HwDecoder {
private:
    volatile int status;
    unsigned int frameWidth;
    unsigned int frameHeight;
    ANativeWindow *surface;
    AMediaCodec* mediaCodec;
public:
    HwDecoder();
    ~HwDecoder();
    inline const unsigned int onStatus() const;
    bool updateSize(unsigned int width, unsigned int height);
    bool setPreview(ANativeWindow *window);
    bool init();
    bool start();
    uint8_t* process(void *in_buffer, size_t raw_size);
    bool stop();
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_HWDECODER_H
