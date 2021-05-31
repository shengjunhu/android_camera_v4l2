//
// Created by Hsj on 2021/5/24.
//

#ifndef ANDROID_CAMERA_V4L2_DECODERJP_H
#define ANDROID_CAMERA_V4L2_DECODERJP_H

#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "cdjpeg.h"
#include "IDecoder.h"

struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;

class DecoderJp: public IDecoder {
private:
    struct my_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
    djpeg_dest_ptr dest_mgr = NULL;
    JSAMPARRAY buffer;
public:
    CONSTRUCT_CLASS(DecoderJp);
    virtual bool create();
    virtual bool start();
    virtual uint8_t* convert(void *raw_buffer, unsigned int raw_size);
    virtual bool stop();
    virtual void destroy();
};

#endif //ANDROID_CAMERA_V4L2_DECODERJP_H
