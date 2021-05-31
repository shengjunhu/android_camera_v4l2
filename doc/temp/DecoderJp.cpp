//
// Created by Hsj on 2021/5/24.
//

#include "DecoderJp.h"

#define TAG "DecoderJp"

void my_error_exit(j_common_ptr cinfo) {
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message)(cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

bool DecoderJp::create() {
    //1-Allocate and initialize JPEG decompression object
    cinfo->err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        destroy();
        return false;
    } else {
        jpeg_create_decompress(cinfo);
        //2-Set parameters for decompression
        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.output_width = width;
        cinfo.output_height = height;
        cinfo.output_components = 3;
        cinfo.out_color_space = JCS_RGB;
        return true;
    }
}

bool DecoderJp::start() {
    return true;
}

uint8_t *DecoderJp::convert(void *raw_buffer, unsigned int raw_size) {
    //3-Input raw buffer
    jpeg_mem_src(&cinfo, raw_buffer, raw_size);
    //4-read default parameters
    jpeg_read_header(&cinfo, TRUE);
    //5-Start decompressor
    jpeg_start_decompress(cinfo);
    //JSAMPLE per row in output buffer
    int row_stride = cinfo->output_width * cinfo->output_components;
    //Make a one-row-high sample array that will go away when done with image
    buffer = (*cinfo->mem->alloc_sarray)((j_common_ptr) cinfo, JPOOL_IMAGE, row_stride, 1);

    

    //6-while(scan lines remain to be read)
    while (cinfo->output_scanline < cinfo->output_height) {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could ask for
         * more than one scanline at a time if that's more convenient.
         */
        jpeg_read_scanlines(cinfo, buffer, 1);
        //Assume put_scanline_someplace wants a pointer and sample count.
        put_scanline_someplace(buffer[0], row_stride);
    }

    return NULL;
}

bool DecoderJp::stop() {
    return true;
}

void DecoderJp::destroy() {
    //8-Release JPEG decompression object
    jpeg_destroy_decompress(cinfo);
}