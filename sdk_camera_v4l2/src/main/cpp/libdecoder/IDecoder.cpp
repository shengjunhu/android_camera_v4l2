//
// Created by Hsj on 2021/5/24.
//

#include "IDecoder.h"

#define TAG "IDecoder"

enum { STATUS_INIT = 0, STATUS_CREATE = 1, STATUS_RUN = 2 };

inline const unsigned int IDecoder::onStatus() const { return status; }

bool IDecoder::create() {
    bool ret = false;
    if (STATUS_INIT == onStatus()) {
        ret = this->create();
        if (ret) status = STATUS_CREATE;
    } else {
        LOGE(TAG, "create status: %d", onStatus())
    }
    return ret;
}

bool IDecoder::start() {
    bool ret = false;
    if (STATUS_CREATE == onStatus()) {
        ret = this->start();
        if (ret) status =  STATUS_RUN;
    } else {
        LOGE(TAG, "start status: %d", onStatus())
    }
    return ret;
}

uint8_t* IDecoder::convert(void *raw_buffer, unsigned long raw_size) {
    uint8_t* out_buffer = NULL;
    if (STATUS_RUN == onStatus()) {
        out_buffer = this->convert(raw_buffer, raw_size);
    } else {
        LOGE(TAG, "convert status: %d", onStatus())
    }
    return out_buffer;
}

bool IDecoder::stop() {
    bool ret = false;
    if (STATUS_RUN == onStatus()) {
        status = STATUS_CREATE;
        ret = this->stop();
    } else {
        LOGE(TAG, "stop status: %d", onStatus())
    }
    return ret;
}

void IDecoder::destroy() {
    stop();
    if (STATUS_CREATE == onStatus()) {
        status = STATUS_INIT;
        this->destroy();
    } else {
        LOGE(TAG, "destroy status: %d", onStatus())
    }
}
