//
// Created by Hsj on 2021/5/31.
//

#include "CameraAPI.h"
#include "Common.h"
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

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "CameraAPI"
#define MAX_BUFFER_COUNT 4
#define MAX_DEV_VIDEO_INDEX 99

CameraAPI::CameraAPI() :
        fd(0),
        pixelBytes(0),
        frameWidth(0),
        frameHeight(0),
        frameFormat(0),
        thread_camera(0),
        status(STATUS_CREATE),
        preview(NULL),
        decoder(NULL),
        buffers(NULL),
        out_buffer(NULL),
        frameCallback(NULL),
        frameCallback_onFrame(NULL) {
}

CameraAPI::~CameraAPI() {
    destroy();
}

//=======================================Private====================================================

inline const StatusInfo CameraAPI::getStatus() const { return status; }

ActionInfo CameraAPI::prepareBuffer() {
    //1-request buffers
    struct v4l2_requestbuffers buffer1;
    //SAFE_CLEAR(buffer1)
    buffer1.count = MAX_BUFFER_COUNT;
    buffer1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer1.memory = V4L2_MEMORY_MMAP;
    if (0 > ioctl(fd, VIDIOC_REQBUFS, &buffer1)) {
        LOGE(TAG, "prepareBuffer: ioctl VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ACTION_ERROR_START;
    }

    //2-query memory
    buffers = (struct VideoBuffer *) calloc(MAX_BUFFER_COUNT, sizeof(*buffers));
    for (unsigned int i = 0; i < MAX_BUFFER_COUNT; ++i) {
        struct v4l2_buffer buffer2;
        //SAFE_CLEAR(buffer2)
        buffer2.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer2.memory = V4L2_MEMORY_MMAP;
        buffer2.index = i;
        if (0 > ioctl(fd, VIDIOC_QUERYBUF, &buffer2)) {
            LOGE(TAG, "prepareBuffer: ioctl VIDIOC_QUERYBUF failed: %s", strerror(errno));
            return ACTION_ERROR_START;
        }
        buffers[i].length = buffer2.length;
        buffers[i].start = mmap(NULL, buffer2.length,PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer2.m.offset);
        if (MAP_FAILED == buffers[i].start) {
            LOGE(TAG, "prepareBuffer: ioctl VIDIOC_QUERYBUF failed2");
            return ACTION_ERROR_START;
        }
    }

    //3-v4l2_buffer
    for (unsigned int i = 0; i < MAX_BUFFER_COUNT; ++i) {
        struct v4l2_buffer buffer3;
        //SAFE_CLEAR(buffer3)
        buffer3.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer3.memory = V4L2_MEMORY_MMAP;
        buffer3.index = i;
        if (0 > ioctl(fd, VIDIOC_QBUF, &buffer3)) {
            LOGE(TAG, "prepareBuffer: ioctl VIDIOC_QBUF failed: %s", strerror(errno));
            return ACTION_ERROR_START;
        }
    }
    return ACTION_SUCCESS;
}

void* CameraAPI::loopThread(void *args) {
    auto *camera = reinterpret_cast<CameraAPI *>(args);
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

//uint64_t time0 = 0;
//uint64_t time1 = 0;

void CameraAPI::loopFrame(JNIEnv *env, CameraAPI *camera) {
    fd_set fds;
    struct timeval tv;
    struct v4l2_buffer buffer;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    const int fd_count = camera->fd + 1;
    while (STATUS_RUN == camera->getStatus()) {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO (&fds);
        FD_SET (camera->fd, &fds);
        if (0 >= select(fd_count, &fds, NULL, NULL, &tv)) {//非阻塞，0超时，-1错误
            LOGE(TAG, "Loop frame failed: %s", strerror(errno));
            continue;
        } else if (0 > ioctl(camera->fd, VIDIOC_DQBUF, &buffer)) {
            //TODO video/dev* disconnect, implement auto connect
            LOGE(TAG, "Loop frame failed2: %s", strerror(errno));
            break;
        } else if (camera->frameFormat == FRAME_FORMAT_MJPEG) {
            //LOGD(TAG, "mjpeg interval time = %ld", timeMs() - time1);
            //time1 = timeMs();

            //MJPEG->NV12/YUV422
            uint8_t *data = camera->decoder->convert2YUV(camera->buffers[buffer.index].start, buffer.bytesused);
            //LOGD(TAG, "decodeTime=%lld", timeMs() - time1)

            //Render->RGBA
            renderFrame(data);

            //Data->Java
            sendFrame(env, data);
        } else {
            //LOGD(TAG, "yuyv interval time = %lld", timeMs() - time0)
            //time0 = timeMs();

            //YUYV
            memcpy(out_buffer, camera->buffers[buffer.index].start, buffer.length);

            //Render->YUYV
            renderFrame(out_buffer);

            //YUYV->Java
            sendFrame(env, out_buffer);
        }
        if (0 > ioctl(camera->fd, VIDIOC_QBUF, &buffer)) {
            LOGW(TAG, "Loop frame: ioctl VIDIOC_QBUF %s", strerror(errno));
            continue;
        }
    }
}

void CameraAPI::renderFrame(uint8_t *data) {
    //u_int64_t start = timeMs();
    if (LIKELY(preview && data)) {
        preview->render(data);
    }
    //LOGD(TAG, "renderTime=%lld", timeMs() - start)
}

void CameraAPI::sendFrame(JNIEnv *env, uint8_t *data) {
    if (frameCallback_onFrame && LIKELY(data)) {
        jobject frame = env->NewDirectByteBuffer(data, pixelBytes);
        env->CallVoidMethod(frameCallback, frameCallback_onFrame, frame);
        env->DeleteLocalRef(frame);
        env->ExceptionClear();
    }
}

//=======================================Public=====================================================

ActionInfo CameraAPI::connect(unsigned int target_pid, unsigned int target_vid) {
    ActionInfo action = ACTION_SUCCESS;
    if (STATUS_CREATE == getStatus()) {
        std::string modalias;
        std::string dev_video_name;
        for (int i = 0; i <= MAX_DEV_VIDEO_INDEX; ++i) {
            int vid = 0, pid = 0;
            dev_video_name.append("video").append(std::to_string(i));
            if (!(std::ifstream("/sys/class/video4linux/" + dev_video_name + "/device/modalias") >> modalias)) {
                LOGD(TAG, "dev/%s : read modalias failed", dev_video_name.c_str());
            } else if (modalias.size() < 14 || modalias.substr(0, 5) != "usb:v" || modalias[9] != 'p') {
                LOGD(TAG, "dev/%s : format is not a usb of modalias", dev_video_name.c_str());
            } else if (!(std::istringstream(modalias.substr(5, 4)) >> std::hex >> vid)) {
                LOGD(TAG, "dev/%s : read vid failed", dev_video_name.c_str());
            } else if (!(std::istringstream(modalias.substr(10, 4)) >> std::hex >> pid)) {
                LOGD(TAG, "dev/%s : read pid failed", dev_video_name.c_str());
            } else {
                LOGD(TAG, "dev/%s : vid=%d, pid=%d", dev_video_name.c_str(), vid, pid);
            }
            if (target_pid == pid && target_vid == vid) {
                dev_video_name.insert(0, "dev/");
                break;
            } else {
                modalias.clear();
                dev_video_name.clear();
            }
        }
        if (dev_video_name.empty()) {
            LOGW(TAG, "connect: no target device");
            action = ACTION_ERROR_NO_DEVICE;
        } else {
            const char *deviceName = dev_video_name.data();
            fd = open(deviceName, O_RDWR | O_NONBLOCK, S_IRWXU);
            if (0 > fd) {
                LOGE(TAG, "open: %s failed, %s", deviceName, strerror(errno));
                action = ACTION_ERROR_OPEN_FAIL;
            } else {
                struct v4l2_capability cap;
                if (0 > ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                    LOGE(TAG, "open: ioctl VIDIOC_QUERYCAP failed, %s", strerror(errno));
                    ::close(fd);
                    action = ACTION_ERROR_START;
                } else {
                    LOGD(TAG, "open: %s succeed", deviceName);
                    status = STATUS_OPEN;
                }
            }
        }
    } else {
        LOGW(TAG, "open: error status, %d", getStatus());
        action = ACTION_ERROR_CREATE_HAD;
    }
    return action;
}

ActionInfo CameraAPI::autoExposure(bool isAuto) {
    if (STATUS_OPEN <= getStatus()) {
        struct v4l2_control ctrl;
        //SAFE_CLEAR(ctrl)
        ctrl.id = V4L2_CID_EXPOSURE_AUTO;
        ctrl.value = isAuto ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;
        if (0 > ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
            LOGW(TAG, "autoExposure: ioctl VIDIOC_S_CTRL failed, %s", strerror(errno));
            return ACTION_ERROR_AUTO_EXPOSURE;
        } else {
            LOGD(TAG, "autoExposure: success");
            return ACTION_SUCCESS;
        }
    } else {
        LOGW(TAG, "autoExposure: error status, %d", getStatus());
        return ACTION_ERROR_AUTO_EXPOSURE;
    }
}

ActionInfo CameraAPI::updateExposure(unsigned int level) {
    if (STATUS_OPEN <= getStatus()) {
        struct v4l2_control ctrl;
        //SAFE_CLEAR(ctrl)
        ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
        ctrl.value = level;
        if (0 > ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
            LOGE(TAG, "updateExposure: ioctl failed, %s", strerror(errno));
            return ACTION_ERROR_SET_EXPOSURE;
        } else {
            LOGD(TAG, "updateExposure: success");
            return ACTION_SUCCESS;
        }
    } else {
        LOGW(TAG, "updateExposure: error status, %d", getStatus());
        return ACTION_ERROR_SET_EXPOSURE;
    }
}

ActionInfo CameraAPI::getSupportSize(std::vector<std::pair<int, int>> &sizes) {
    if (STATUS_OPEN <= getStatus()) {
        struct v4l2_frmsizeenum frmsize;
        struct v4l2_fmtdesc fmtdesc;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmtdesc.index = 0;
        while (ioctl(fd,VIDIOC_ENUM_FMT, &fmtdesc) == 0){
            frmsize.pixel_format = fmtdesc.pixelformat;
            frmsize.index = 0;
            while (ioctl(fd,VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
                if (fmtdesc.flags == V4L2_FMT_FLAG_COMPRESSED){ //Compressed size
                    if (frmsize.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                        sizes.emplace_back(frmsize.discrete.width, frmsize.discrete.height);
                    }  else {
                        LOGE(TAG, "getSupportSize(): type=%d", frmsize.type);
                    }
                } else { //UnCompressed size
                    if (frmsize.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                        //sizes.emplace_back(frmsize.discrete.width, frmsize.discrete.height);
                    }  else {
                        LOGE(TAG, "getSupportSize(): type=%d", frmsize.type);
                    }
                }
                frmsize.index++;
            }
            fmtdesc.index++;
        }
        return ACTION_SUCCESS;
    } else {
        LOGW(TAG, "getSupportSize: error status, %d", getStatus());
        return ACTION_ERROR_GET_W_H;
    }
}

ActionInfo CameraAPI::setFrameSize(int width, int height, int frame_format) {
    if (STATUS_OPEN == getStatus()) {
        //1-set frame width and height
        struct v4l2_format format;
        //SAFE_CLEAR(format)
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width = width;
        format.fmt.pix.height = height;
        format.fmt.pix.field = V4L2_FIELD_ANY;
        format.fmt.pix.pixelformat = frame_format ? V4L2_PIX_FMT_YUYV : V4L2_PIX_FMT_MJPEG;
        if (0 > ioctl(fd, VIDIOC_S_FMT, &format)) {
            LOGW(TAG, "setFrameSize: ioctl set format failed, %s", strerror(errno));
            return ACTION_ERROR_SET_W_H;
        }
        if (frame_format) { // YUYV
            pixelBytes = width * height * 2;
            out_buffer = (uint8_t *) calloc(1, pixelBytes);
        } else { // MJPEG
            decoder = new DecoderFactory();
            if (0 != decoder->init(width, height)){
                SAFE_DELETE(decoder);
                LOGE(TAG, "DecoderFactory init failed");
                return ACTION_ERROR_DECODER;
            } else if (PIXEL_FORMAT_NV12 == decoder->getPixelFormat()) {
                pixelBytes = width * height * 3 / 2;
            } else {
                pixelBytes = width * height * 2;
            }
        }

        //2-set frame fps
        struct v4l2_streamparm parm;
        //SAFE_CLEAR(parm)
        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = 1;
        if (frame_format == FRAME_FORMAT_MJPEG) {
            parm.parm.capture.timeperframe.denominator = 30;
        } else {
            parm.parm.capture.timeperframe.denominator = 10;
        }
        if (0 > ioctl(fd, VIDIOC_S_PARM, &parm)) {
            LOGW(TAG, "setFrameSize: ioctl set fps failed, %s", strerror(errno));
        }

        //3-what function ?
        unsigned int min = format.fmt.pix.width * 2;
        if (format.fmt.pix.bytesperline < min) {
            format.fmt.pix.bytesperline = min;
        }
        min = format.fmt.pix.bytesperline * format.fmt.pix.height;
        if (format.fmt.pix.sizeimage < min) {
            format.fmt.pix.sizeimage = min;
        }

        frameWidth = width;
        frameHeight = height;
        frameFormat = frame_format;
        status = STATUS_INIT;
        return ACTION_SUCCESS;
    } else {
        LOGW(TAG, "setFrameSize: error status, %d", getStatus());
        return ACTION_ERROR_SET_W_H;
    }
}

ActionInfo CameraAPI::setFrameCallback(JNIEnv *env, jobject frame_callback) {
    if (STATUS_INIT == getStatus()) {
        if (!env->IsSameObject(frameCallback, frame_callback)) {
            if (frameCallback) {
                env->DeleteGlobalRef(frameCallback);
            }
            if (frame_callback) {
                jclass clazz = env->GetObjectClass(frame_callback);
                if (LIKELY(clazz)) {
                    frameCallback = frame_callback;
                    frameCallback_onFrame = env->GetMethodID(clazz, "onFrame","(Ljava/nio/ByteBuffer;)V");
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
        LOGW(TAG, "setFrameCallback: error status, %d", getStatus());
        return ACTION_ERROR_CALLBACK;
    }
}

ActionInfo CameraAPI::setPreview(ANativeWindow *window) {
    if (STATUS_INIT == getStatus()) {
        if (preview != NULL) {
            preview->destroy();
            SAFE_DELETE(preview);
        }
        if (LIKELY(window != NULL)) {
            PixelFormat pixelFormat = PIXEL_FORMAT_ERROR;
            if (decoder != NULL) {
                pixelFormat = decoder->getPixelFormat();
            } else if (frameFormat == FRAME_FORMAT_YUYV) {
                pixelFormat = PIXEL_FORMAT_YUYV;
            } else if (frameFormat == FRAME_FORMAT_DEPTH) {
                pixelFormat = PIXEL_FORMAT_DEPTH;
            }
            preview = new CameraView(frameWidth, frameHeight, pixelFormat, window);
        }
        return ACTION_SUCCESS;
    } else {
        LOGW(TAG, "setPreview: error status, %d", getStatus());
        return ACTION_ERROR_SET_PREVIEW;
    }
}

ActionInfo CameraAPI::start() {
    ActionInfo action = ACTION_ERROR_START;
    if (STATUS_INIT == getStatus()) {
        if (ACTION_SUCCESS == prepareBuffer()) {
            //1-start stream
            enum v4l2_buf_type type;
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (0 > ioctl(fd, VIDIOC_STREAMON, &type)) {
                LOGE(TAG, "start: ioctl VIDIOC_STREAMON failed, %s", strerror(errno));
            } else {
                status = STATUS_RUN;
                //3-start thread loop frame
                if (0 == pthread_create(&thread_camera, NULL, loopThread, (void *) this)) {
                    LOGD(TAG, "start: success");
                    action = ACTION_SUCCESS;
                } else {
                    LOGE(TAG, "start: pthread_create failed");
                }
            }
        } else {
            LOGE(TAG, "start: error prepare buffer, %d", getStatus());
        }
    } else {
        LOGW(TAG, "start: error status, %d", getStatus());
    }
    return action;
}

ActionInfo CameraAPI::stop() {
    ActionInfo action = ACTION_SUCCESS;
    if (STATUS_RUN == getStatus()) {
        status = STATUS_INIT;
        //1-stop thread
        if (0 == pthread_join(thread_camera, NULL)) {
            LOGD(TAG, "stop: pthread_join success");
        } else {
            LOGE(TAG, "stop: pthread_join failed, %s", strerror(errno));
            action = ACTION_ERROR_STOP;
        }
        //3-stop preview
        if (preview) preview->pause();
        //4-stop stream
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (0 > ioctl(fd, VIDIOC_STREAMOFF, &type)) {
            LOGE(TAG, "stop: ioctl failed: %s", strerror(errno));
            action = ACTION_ERROR_STOP;
        } else {
            LOGD(TAG, "stop: ioctl VIDIOC_STREAMOFF success");
        }
        //5-release buffer
        for (int i = 0; i < MAX_BUFFER_COUNT; ++i) {
            if (0 != munmap(buffers[i].start, buffers[i].length)) {
                LOGW(TAG, "stop: munmap failed");
            }
        }
    } else {
        LOGW(TAG, "stop: error status, %d", getStatus());
        action = ACTION_ERROR_STOP;
    }
    return action;
}

ActionInfo CameraAPI::close() {
    ActionInfo action = ACTION_SUCCESS;
    if (STATUS_INIT == getStatus()) {
        status = STATUS_CREATE;
        //1-close fd
        if (0 > ::close(fd)) {
            LOGE(TAG, "close: failed, %s", strerror(errno));
            action = ACTION_ERROR_CLOSE;
        } else {
            LOGD(TAG, "close: success");
        }
        //2-release buffer
        SAFE_FREE(buffers)
        SAFE_FREE(out_buffer)
        //3-destroy decoder
        SAFE_DELETE(decoder)
        //4-preview destroy
        if (preview != NULL) {
            preview->destroy();
            SAFE_DELETE(preview);
        }
        //5-release frameCallback
        JNIEnv *env = getEnv();
        if (env && frameCallback_onFrame) {
            env->DeleteGlobalRef(frameCallback);
            frameCallback_onFrame = NULL;
            frameCallback = NULL;
        }
    } else {
        LOGW(TAG, "close: error status, %d", getStatus());
    }
    return action;
}

ActionInfo CameraAPI::destroy() {
    fd = 0;
    pixelBytes = 0;
    frameWidth = 0;
    frameHeight = 0;
    frameFormat = 0;
    thread_camera = 0;
    status = STATUS_CREATE;
    frameCallback = NULL;
    frameCallback_onFrame = NULL;
    SAFE_FREE(buffers)
    SAFE_FREE(out_buffer)
    SAFE_DELETE(decoder)
    LOGD(TAG, "destroy");
    return ACTION_SUCCESS;
}

#ifdef __cplusplus
}  // extern "C"
#endif