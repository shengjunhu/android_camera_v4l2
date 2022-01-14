//
// Created by Hsj on 2021/5/11.
//

#include "Common.h"
#include <ctime>

#ifdef __cplusplus
extern "C" {
#endif

static JavaVM *jvm;

uint64_t timeMs() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (uint64_t) time.tv_sec * 1000 + time.tv_usec / 1000;
}

uint64_t timeUs() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (uint64_t) time.tv_sec * 1000000 + time.tv_usec;
}

void setVM(JavaVM *vm) {
    jvm = vm;
}

JavaVM *getVM() {
    return jvm;
}

JNIEnv *getEnv() {
    JNIEnv *env = nullptr;
    if (jvm == nullptr) {
        return nullptr;
    } else if (JNI_OK == jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        return env;
    } else {
        return nullptr;
    }
}

#ifdef __cplusplus
}  // extern "C"
#endif