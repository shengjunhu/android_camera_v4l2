package com.hsj.camera;

import android.util.Log;
import android.util.Pair;
import android.view.Surface;

import java.util.ArrayList;
import java.util.List;

/**
 * @Author:Hsj
 * @Date:2020-08-31
 * @Class:CameraAPI
 * @Desc:CameraAPI
 */
public final class CameraAPI {

    private static final String TAG = "Camera";
    //FrameFormat
    public static final int FRAME_FORMAT_MJPEG = 0;
    public static final int FRAME_FORMAT_YUYV = 1;
    public static final int FRAME_FORMAT_DEPTH = 2;
    //Status
    private static final int STATUS_ERROR_DESTROYED = 50;
    private static final int STATUS_ERROR_OPEN = 40;
    private static final int STATUS_ERROR_SIZE = 30;
    private static final int STATUS_ERROR_START = 20;
    private static final int STATUS_ERROR_STOP = 10;
    private static final int STATUS_SUCCESS = 0;

    static {
        System.loadLibrary("camera");
    }

//======================================Java API====================================================

    private long nativeObj;

    public CameraAPI() {
        this.nativeObj = nativeInit();
    }

    public final synchronized boolean create(int productId, int vendorId) {
        if (this.nativeObj == 0) {
            Log.e(TAG, "Can't be call after call destroy");
            return false;
        } else {
            int status = nativeCreate(this.nativeObj, productId, vendorId);
            Logger.d(TAG, "create: " + status);
            return STATUS_SUCCESS == status;
        }
    }

    public final boolean setAutoExposure(boolean isAuto) {
        if (this.nativeObj == 0) {
            Log.w(TAG, "Can't be call after call destroy");
            return false;
        } else {
            int status = nativeAutoExposure(this.nativeObj, isAuto);
            Logger.d(TAG, "setAutoExposure: " + status);
            return STATUS_SUCCESS == status;
        }
    }

    public final boolean setExposureLevel(int level) {
        if (this.nativeObj == 0) {
            Log.w(TAG, "Can't be call after call destroy");
            return false;
        } else {
            int status = nativeSetExposure(this.nativeObj, level);
            Logger.d(TAG, "setExposureLevel: " + status);
            return STATUS_SUCCESS == status;
        }
    }

    public final int[][] getSupportFrameSize() {
        int[][] sizes = null;
        if (this.nativeObj != 0) {
            sizes = nativeSupportSize(this.nativeObj);
            int length = (sizes == null ? 0 : sizes.length);
            if (length > 0){
                Logger.d(TAG, "getSupportFrameSize: " + length);
            } else {
                Logger.e(TAG, "getSupportFrameSize: empty");
            }
        } else {
            Logger.e(TAG, "getSupportFrameSize: already destroyed");
        }
        return sizes;
    }

    public final boolean setFrameSize(int width, int height, int frameFormat) {
        if (this.nativeObj == 0) {
            Log.w(TAG, "Can't be call after call destroy");
            return false;
        } else {
            int status = nativeFrameSize(this.nativeObj, width, height, frameFormat);
            Logger.d(TAG, "setFrameSize: " + status);
            return STATUS_SUCCESS == status;
        }
    }

    public final boolean setFrameCallback(IFrameCallback frameCallback) {
        if (this.nativeObj == 0) {
            Log.w(TAG, "Can't be call after call destroy");
            return false;
        } else {
            int status = nativeFrameCallback(this.nativeObj, frameCallback);
            Logger.d(TAG, "setFrameCallback: " + status);
            return STATUS_SUCCESS == status;
        }
    }

    public final boolean setPreview(Surface surface) {
        if (this.nativeObj == 0) {
            Log.w(TAG, "Can't be call after call setPreview");
            return false;
        } else {
            int status = nativePreview(this.nativeObj, surface);
            Logger.d(TAG, "setPreview: " + status);
            return STATUS_SUCCESS == status;
        }
    }

    public final synchronized boolean start() {
        if (this.nativeObj == 0) {
            Log.w(TAG, "Can't be call after call destroy");
            return false;
        } else {
            int status = nativeStart(this.nativeObj);
            Logger.d(TAG, "start: " + status);
            return STATUS_SUCCESS == status;
        }
    }

    public final synchronized boolean stop() {
        if (this.nativeObj == 0) {
            Log.w(TAG, "Can't be call after call destroy");
            return false;
        } else {
            int status = nativeStop(this.nativeObj);
            Logger.d(TAG, "stop: " + status);
            return STATUS_SUCCESS == status;
        }
    }

    public final synchronized void destroy() {
        if (this.nativeObj == 0) {
            Logger.w(TAG, "destroy: already destroyed");
        } else {
            int status = nativeDestroy(this.nativeObj);
            Logger.w(TAG, "destroy: " + status);
            this.nativeObj = 0;
        }
    }

//=======================================Native API=================================================

    private native long nativeInit();

    private native int nativeCreate(long nativeObj, int productId, int vendorId);

    private native int nativeAutoExposure(long nativeObj, boolean isAuto);

    private native int nativeSetExposure(long nativeObj, int level);

    private native int[][] nativeSupportSize(long nativeObj);

    private native int nativeFrameSize(long nativeObj, int width, int height, int pixelFormat);

    private native int nativeFrameCallback(long nativeObj, IFrameCallback frameCallback);

    private native int nativePreview(long nativeObj, Surface surface);

    private native int nativeStart(long nativeObj);

    private native int nativeStop(long nativeObj);

    private native int nativeDestroy(long nativeObj);

}
