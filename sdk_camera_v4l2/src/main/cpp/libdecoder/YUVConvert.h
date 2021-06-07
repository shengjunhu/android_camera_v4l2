//
// Created by Hsj on 2021/6/4.
//

#ifndef ANDROID_CAMERA_V4L2_YUVCONVERT_H
#define ANDROID_CAMERA_V4L2_YUVCONVERT_H

#include <turbojpeg.h>
#include "DecodeCreator.h"

class YUVConvert {
private:
    int flags;
    int subSample;
    int pixel_wh;
    int pixel_width;
    int pixel_height;
    int pixel_format;
    tjhandle handle;
    uint8_t *out_buffer1;
    unsigned char *out_buffer2;
public:
    YUVConvert(int pixelWidth, int pixelHeight, PixelFormat pixelFormat);
    ~YUVConvert();
    uint8_t* convertNV12(const uint8_t *data);
    unsigned char* convertYUV422(const uint8_t *data);
    void destroy();
};

#endif //ANDROID_CAMERA_V4L2_YUVCONVERT_H
