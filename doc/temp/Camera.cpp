//
// Created by Hsj on 2021/1/22.
//

#include "Common.h"
#include "Camera.h"
#include <malloc.h>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define TAG "Camera"
#define MAX_BUFFER_COUNT 4
#define MAX_DEV_VIDEO_INDEX 9

Camera::Camera() :
        fd(0),
        pixelBytes(0),
        frameWidth(0),
        frameHeight(0),
        pixelFormat(0),
        thread_camera(0),
        status(STATUS_READY),
        outBuf(NULL),
        buffers(NULL),
        hwDecoder(NULL),
        deviceName(NULL),
        frameCallback(NULL),
        frameCallback_onFrame(NULL) {
}

Camera::~Camera() {
    destroy();
}

//=======================================Private====================================================

inline const StatusInfo Camera::getStatus() const { return status; }

ActionInfo Camera::prepareBuffer() {
    //1-request buffers
    struct v4l2_requestbuffers buffer1;
    SAFE_CLEAR(buffer1)
    buffer1.count = MAX_BUFFER_COUNT;
    buffer1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer1.memory = V4L2_MEMORY_MMAP;
    if (0 > ioctl(fd, VIDIOC_REQBUFS, &buffer1)) {
        LOGE(TAG, "prepareBuffer: ioctl VIDIOC_REQBUFS failed: %s", strerror(errno))
        return ACTION_ERROR_START;
    }

    //2-query memory
    buffers = (struct VideoBuffer *) calloc(MAX_BUFFER_COUNT, sizeof(*buffers));
    for (unsigned int i = 0; i < MAX_BUFFER_COUNT; ++i) {
        struct v4l2_buffer buffer2;
        SAFE_CLEAR(buffer2)
        buffer2.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer2.memory = V4L2_MEMORY_MMAP;
        buffer2.index = i;
        if (0 > ioctl(fd, VIDIOC_QUERYBUF, &buffer2)) {
            LOGE(TAG, "prepareBuffer: ioctl VIDIOC_QUERYBUF failed: %s", strerror(errno))
            return ACTION_ERROR_START;
        }
        buffers[i].length = buffer2.length;
        buffers[i].start = mmap(NULL, buffer2.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                                buffer2.m.offset);
        if (MAP_FAILED == buffers[i].start) {
            LOGE(TAG, "prepareBuffer: ioctl VIDIOC_QUERYBUFfailed2")
            return ACTION_ERROR_START;
        }
    }

    //3-v4l2_buffer
    for (unsigned int i = 0; i < MAX_BUFFER_COUNT; ++i) {
        struct v4l2_buffer buffer3;
        SAFE_CLEAR(buffer3)
        buffer3.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer3.memory = V4L2_MEMORY_MMAP;
        buffer3.index = i;
        if (0 > ioctl(fd, VIDIOC_QBUF, &buffer3)) {
            LOGE(TAG, "prepareBuffer: ioctl VIDIOC_QBUF failed: %s", strerror(errno))
            return ACTION_ERROR_START;
        }
    }
    return ACTION_SUCCESS;
}

void *Camera::loopThread(void *args) {
    auto *camera = reinterpret_cast<Camera *>(args);
    if (LIKELY(camera)) {
        JavaVM *vm = getVM();
        JNIEnv *env;
        // attach to JavaVM
        vm->AttachCurrentThread(&env, NULL);
        // never return until finish previewing
        camera->loopFrame(env, camera);
        // detach from JavaVM
        vm->DetachCurrentThread();
    }
    pthread_exit(NULL);
}

long long time0 = 0;
long long time1 = 0;

void Camera::loopFrame(JNIEnv *env, Camera *camera) {
    const int fd_count = camera->fd + 1;
    struct v4l2_buffer buffer;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    while (STATUS_START == camera->getStatus()) {
        fd_set fds;
        FD_ZERO (&fds);
        FD_SET (camera->fd, &fds);
        struct timeval tv;
        tv.tv_sec = 2000;
        tv.tv_usec = 0;
        if (0 >= select(fd_count, &fds, NULL, NULL, &tv)) {
            LOGW(TAG, "Loop frame failed: %s", strerror(errno))
            continue;
        } else if (0 > ioctl(camera->fd, VIDIOC_DQBUF, &buffer)) {
            LOGW(TAG, "Loop frame failed: %s", strerror(errno))
            continue;
        } else if (camera->pixelFormat == 0) {
            //LOGD(TAG, "yuyv interval time = %lld", timeMs() - time0)
            //time0 = timeMs();
            memcpy(outBuf, camera->buffers[buffer.index].start, buffer.length);
            //YUYV->Java
            sendFrame(env, outBuf);
        } else {
            //LOGD(TAG, "mjpeg interval time = %lld", timeMs() - time1)
            //time1 = timeMs();

            //MJPG->NV12
            uint8_t *out = camera->hwDecoder->process(camera->buffers[buffer.index].start,buffer.bytesused);
            //LOGD(TAG, "mjpeg2yuv: %lld", timeMs() - time1)

            //SAVE->NV12
            /*FILE *fp = fopen("/sdcard/nv12_1280x800.yuv", "wb");
            if (fp) {
               fwrite(out, 1,  camera->frameWidth*camera->frameHeight*3/2,fp);
               fclose(fp);
               LOGD(TAG,"Capture one frame saved in /sdcard/nv12_1280x800.yuv");
            } else {
               LOGE(TAG,"Loop frame failed: ir")
            }*/

            //NV12->Java
            sendFrame(env, out);
        }
        if (0 > ioctl(camera->fd, VIDIOC_QBUF, &buffer)) {
            LOGW(TAG, "Loop frame: ioctl VIDIOC_QBUF %s", strerror(errno))
            continue;
        }
    }
}

void Camera::sendFrame(JNIEnv *env, uint8_t *data) {
    if (LIKELY(data) && LIKELY(frameCallback_onFrame)) {
        jobject frame = env->NewDirectByteBuffer(data, pixelBytes);
        env->CallVoidMethod(frameCallback, frameCallback_onFrame, frame);
        env->DeleteLocalRef(frame);
        env->ExceptionClear();
    }
}

//=======================================Public=====================================================

ActionInfo Camera::open(unsigned int target_pid, unsigned int target_vid) {
    ActionInfo action = ACTION_SUCCESS;
    if (STATUS_CREATE != getStatus()) {
        std::string dev_video_name;
        for (int i = 0; i <= MAX_DEV_VIDEO_INDEX; ++i) {
            dev_video_name.append("video").append(std::to_string(i));
            std::string modalias;
            int vid = 0, pid = 0;
            if (!(std::ifstream("/sys/class/video4linux/" + dev_video_name + "/device/modalias")
                    >> modalias)) {
                LOGD(TAG, "dev/%s : read modalias failed", dev_video_name.c_str())
            } else if (modalias.size() < 14 || modalias.substr(0, 5) != "usb:v" ||
                       modalias[9] != 'p') {
                LOGD(TAG, "dev/%s : format is not a usb of modalias", dev_video_name.c_str())
            } else if (!(std::istringstream(modalias.substr(5, 4)) >> std::hex >> vid)) {
                LOGD(TAG, "dev/%s : read vid failed", dev_video_name.c_str())
            } else if (!(std::istringstream(modalias.substr(10, 4)) >> std::hex >> pid)) {
                LOGD(TAG, "dev/%s : read pid failed", dev_video_name.c_str())
            } else {
                LOGD(TAG, "dev/%s : vid=%d, pid=%d", dev_video_name.c_str(), vid, pid)
            }
            if (target_pid == pid && target_vid == vid) {
                dev_video_name.insert(0, "dev/");
                break;
            } else {
                dev_video_name.clear();
            }
        }
        if (dev_video_name.empty()) {
            LOGW(TAG, "open: no target device")
            action = ACTION_ERROR_NO_DEVICE;
        } else {
            deviceName = dev_video_name.data();
            fd = ::open(deviceName, O_RDWR | O_NONBLOCK, 0);
            if (0 > fd) {
                LOGE(TAG, "open: %s failed, %s", deviceName, strerror(errno))
                action = ACTION_ERROR_OPEN_FAIL;
            } else {
                struct v4l2_capability cap;
                if (0 > ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                    LOGE(TAG, "open: ioctl VIDIOC_QUERYCAP failed, %s", strerror(errno))
                    ::close(fd);
                    action = ACTION_ERROR_START;
                } else {
                    LOGD(TAG, "open: %s succeed", deviceName)
                    status = STATUS_OPEN;
                }
            }
        }
    } else {
        LOGW(TAG, "open: error status, %d", getStatus())
        action = ACTION_ERROR_CREATE_HAD;
    }
    return action;
}

ActionInfo Camera::autoExposure(bool isAuto) {
    if (STATUS_OPEN == getStatus()) {
        struct v4l2_control ctrl;
        SAFE_CLEAR(ctrl)
        ctrl.id = V4L2_CID_EXPOSURE_AUTO;
        ctrl.value = isAuto ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;
        if (0 > ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
            LOGW(TAG, "autoExposure: ioctl VIDIOC_S_CTRL failed, %s", strerror(errno))
            return ACTION_ERROR_AUTO_EXPOSURE;
        } else {
            LOGD(TAG, "autoExposure: success")
            return ACTION_SUCCESS;
        }
    } else {
        LOGW(TAG, "autoExposure: error status, %d", getStatus())
        return ACTION_ERROR_AUTO_EXPOSURE;
    }
}

ActionInfo Camera::updateExposure(unsigned int level) {
    if (STATUS_CREATE < getStatus()) {
        struct v4l2_control ctrl;
        SAFE_CLEAR(ctrl)
        ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
        ctrl.value = level;
        if (0 > ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
            LOGE(TAG, "updateExposure: ioctl failed, %s", strerror(errno))
            return ACTION_ERROR_SET_EXPOSURE;
        } else {
            LOGD(TAG, "updateExposure: success")
            return ACTION_SUCCESS;
        }
    } else {
        LOGW(TAG, "updateExposure: error status, %d", getStatus())
        return ACTION_ERROR_SET_EXPOSURE;
    }
}

ActionInfo
Camera::setFrameSize(unsigned int width, unsigned int height, unsigned int pixel_format) {
    if (STATUS_OPEN == getStatus()) {
        frameWidth = width;
        frameHeight = height;
        pixelFormat = pixel_format;

        //1-设置分辨率格式
        struct v4l2_format format;
        SAFE_CLEAR(format)
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width = frameWidth;
        format.fmt.pix.height = frameHeight;
        format.fmt.pix.field = V4L2_FIELD_ANY;
        if (pixelFormat == 0) {
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        } else {
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        }
        if (0 > ioctl(fd, VIDIOC_S_FMT, &format)) {
            LOGW(TAG, "setFrameSize: ioctl set format failed, %s", strerror(errno))
            return ACTION_ERROR_SET_W_H;
        } else {
            if (pixelFormat == 0) {
                pixelBytes = frameWidth * frameHeight * 2;
                outBuf = (uint8_t *) calloc(1, pixelBytes);
                if (hwDecoder != NULL) {
                    hwDecoder->stop();
                    hwDecoder->destroy();
                    SAFE_DELETE(hwDecoder)
                }
            } else {
                pixelBytes = frameWidth * frameHeight * 3 / 2;
                if (hwDecoder == NULL) {
                    hwDecoder = new HwDecoder();
                }
                hwDecoder->updateSize(frameWidth, frameHeight);
            }
        }

        //2-设置帧率
        struct v4l2_streamparm parm;
        SAFE_CLEAR(parm)
        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = 1;
        if (pixelFormat == 0) {
            parm.parm.capture.timeperframe.denominator = 10;
        } else {
            parm.parm.capture.timeperframe.denominator = 30;
        }
        if (0 > ioctl(fd, VIDIOC_S_PARM, &parm)) {
            LOGW(TAG, "setFrameSize: ioctl set fps failed, %s", strerror(errno))
        }

        //???
        unsigned int min = format.fmt.pix.width * 2;
        if (format.fmt.pix.bytesperline < min) {
            format.fmt.pix.bytesperline = min;
        }
        min = format.fmt.pix.bytesperline * format.fmt.pix.height;
        if (format.fmt.pix.sizeimage < min) {
            format.fmt.pix.sizeimage = min;
        }

        status = STATUS_PARAM;
        return ACTION_SUCCESS;
    } else {
        LOGW(TAG, "setFrameSize: error status, %d", getStatus())
        return ACTION_ERROR_SET_W_H;
    }
}

ActionInfo Camera::setFrameCallback(JNIEnv *env, jobject frame_callback) {
    if (STATUS_PARAM == getStatus()) {
        if (!env->IsSameObject(frameCallback, frame_callback)) {
            if (frameCallback) {
                env->DeleteGlobalRef(frameCallback);
            }
            if (frame_callback) {
                jclass clazz = env->GetObjectClass(frame_callback);
                if (LIKELY(clazz)) {
                    frameCallback = frame_callback;
                    frameCallback_onFrame = env->GetMethodID(clazz, "onFrame",
                                                             "(Ljava/nio/ByteBuffer;)V");
                }
                env->ExceptionClear();
                if (!frameCallback_onFrame) {
                    env->DeleteGlobalRef(frameCallback);
                    frameCallback = NULL;
                    frameCallback_onFrame = NULL;
                }
            }
        }
        return ACTION_SUCCESS;
    } else {
        LOGW(TAG, "setFrameCallback: error status, %d", getStatus())
        return ACTION_ERROR_CALLBACK;
    }
}

ActionInfo Camera::start() {
    ActionInfo action = ACTION_ERROR_START;
    if (STATUS_PARAM == getStatus()) {
        if (ACTION_SUCCESS == prepareBuffer()) {
            //1-开始获取流
            enum v4l2_buf_type type;
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (0 > ioctl(fd, VIDIOC_STREAMON, &type)) {
                LOGE(TAG, "start: ioctl VIDIOC_STREAMON failed, %s", strerror(errno))
            } else {
                status = STATUS_START;
                //2-启动解码器
                if (hwDecoder) {
                    if (hwDecoder->init()) {
                        hwDecoder->start();
                    }
                }
                //3-启动轮询线程
                if (0 == pthread_create(&thread_camera, NULL, loopThread, (void *) this)) {
                    LOGD(TAG, "start: success")
                    action = ACTION_SUCCESS;
                } else {
                    LOGE(TAG, "start: pthread_create failed")
                }
            }
        } else {
            LOGE(TAG, "start: error prepare buffer, %d", getStatus())
        }
    } else {
        LOGW(TAG, "start: error status, %d", getStatus())
    }
    return action;
}

ActionInfo Camera::stop() {
    ActionInfo action = ACTION_SUCCESS;
    if (STATUS_START == getStatus()) {
        status = STATUS_OPEN;
        //1-stop thread
        if (0 == pthread_join(thread_camera, NULL)) {
            LOGD(TAG, "stop: pthread_join success")
        } else {
            LOGE(TAG, "stop: pthread_join failed, %s", strerror(errno))
            action = ACTION_ERROR_STOP;
        }
        //2-stop stream
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (0 > ioctl(fd, VIDIOC_STREAMOFF, &type)) {
            LOGE(TAG, "stop: ioctl failed: %s", strerror(errno))
            action = ACTION_ERROR_STOP;
        } else {
            LOGD(TAG, "stop: ioctl VIDIOC_STREAMOFF success")
        }
        //3-release buffer
        for (unsigned int i = 0; i < MAX_BUFFER_COUNT; ++i) {
            if (0 != munmap(buffers[i].start, buffers[i].length)) {
                LOGW(TAG, "stop: munmap failed")
            }
        }
        //4-stop decoder
        if (hwDecoder) {
            hwDecoder->stop();
        }
        SAFE_FREE(buffers)
    } else {
        LOGW(TAG, "stop: error status, %d", getStatus())
        action = ACTION_ERROR_STOP;
    }
    return action;
}

ActionInfo Camera::close() {
    ActionInfo action = ACTION_SUCCESS;
    if (STATUS_OPEN == getStatus()) {
        status = STATUS_CREATE;
        //1-close fd
        if (0 > ::close(fd)) {
            LOGE(TAG, "close: failed, %s", strerror(errno))
            action = ACTION_ERROR_CLOSE;
        } else {
            LOGD(TAG, "close: success")
        }
        //2-destroy hwDecoder
        if (hwDecoder) {
            hwDecoder->destroy();
            SAFE_DELETE(hwDecoder)
        }
        //3-release frameCallback
        JNIEnv *env = getEnv();
        if (env && !frameCallback_onFrame) {
            env->DeleteGlobalRef(frameCallback);
            frameCallback = NULL;
            frameCallback_onFrame = NULL;
        }
    } else {
        LOGW(TAG, "close: error status, %d", getStatus())
    }
    return action;
}

ActionInfo Camera::destroy() {
    LOGD(TAG, "destroy")
    fd = 0;
    pixelBytes = 0;
    frameWidth = 0;
    frameHeight = 0;
    pixelFormat = 0;
    thread_camera = 0;
    status = STATUS_READY;
    frameCallback_onFrame = NULL;
    frameCallback = NULL;
    SAFE_FREE(outBuf)
    SAFE_FREE(buffers)
    SAFE_DELETE(hwDecoder)
    SAFE_DELETE(deviceName)
    return ACTION_SUCCESS;
}

