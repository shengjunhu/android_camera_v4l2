//
// Created by Hsj on 2021/5/11.
//

#include <jni.h>
#include "Common.h"

extern jint registerAPI(JNIEnv *env);

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (JNI_OK == vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)){
        jint ret = registerAPI(env);
        setVM(vm);
        return ret;
    } else {
        return JNI_ERR;
    }
}