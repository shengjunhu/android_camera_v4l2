package com.hsj.camera;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.WindowManager;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:CameraView
 * @Desc:
 */
public final class CameraView extends GLSurfaceView implements GLSurfaceView.Renderer {

    private static final String TAG = "CameraView";
    public static final int YUYV  = 0;
    public static final int NV12  = 1;
    public static final int DEPTH = 2;
    public static final int RGB   = 3;
    private IRender render;

    public CameraView(Context context) {
        this(context, null);
    }

    public CameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setZOrderOnTop(true);
        setZOrderMediaOverlay(true);
    }

    public IRender getRender(int frameW, int frameH, int frameFormat) {
        if (frameW < 1 || frameH < 1) {
            throw new IllegalArgumentException("Frame width and height is unavailable");
        } else {
            setEGLContextClientVersion(2);
            setRenderer(this);
            setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
            setDebugFlags(DEBUG_CHECK_GL_ERROR | DEBUG_LOG_GL_CALLS);
            if (frameFormat == NV12) {
                render = new RenderNV12(this, frameW, frameH);
            } else if (frameFormat == YUYV) {
                render = new RenderYUYV(this, frameW, frameH);
            } else if (frameFormat == DEPTH) {
                render = new RenderDEPTH(this, frameW, frameH);
            } else if (frameFormat == RGB) {
                render = new RenderRGB(this, frameW, frameH);
            } else {
                throw new IllegalArgumentException("Not support render format: " + frameFormat);
            }
        }
        return render;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        if (render != null) render.onSurfaceCreated(gl, config);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        if (render != null) render.onSurfaceChanged(gl, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        if (render != null) render.onDrawFrame(gl);
    }
}
