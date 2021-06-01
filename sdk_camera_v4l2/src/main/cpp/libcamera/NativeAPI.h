//
// Created by Hsj on 2021/4/26.
//

#ifndef ANDROID_CAMERA_V4L2_NATIVEAPI_H
#define ANDROID_CAMERA_V4L2_NATIVEAPI_H

typedef enum ActionInfo{
    ACTION_SUCCESS                = 0,
    ACTION_ERROR_CREATE           = 9,
    ACTION_ERROR_CREATE_HAD       = 10,
    ACTION_ERROR_NO_DEVICE        = 18,
    ACTION_ERROR_OPEN_FAIL        = 19,
    ACTION_ERROR_OPEN             = 20,
    ACTION_ERROR_AUTO_EXPOSURE    = 25,
    ACTION_ERROR_SET_EXPOSURE     = 26,
    ACTION_ERROR_SET_W_H          = 27,
    ACTION_ERROR_SET_PREVIEW      = 28,
    ACTION_ERROR_CALLBACK         = 29,
    ACTION_ERROR_START            = 30,
    ACTION_ERROR_STOP             = 40,
    ACTION_ERROR_CLOSE            = 50,
    ACTION_ERROR_DESTROY          = 60,
    ACTION_ERROR_RELEASE          = 70
}actionInfo;

#endif //ANDROID_CAMERA_V4L2_NATIVEAPI_H
