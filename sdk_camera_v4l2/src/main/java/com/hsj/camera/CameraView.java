package com.hsj.camera;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

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
    public static final int COMMON = 0;
    public static final int BEAUTY = 1;
    public static final int DEPTH  = 2;
    private IRender render;

    public CameraView(Context context) {
        this(context, null);
    }

    public CameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setZOrderOnTop(true);
        setZOrderMediaOverlay(true);
    }

    public IRender getRender(int frameW, int frameH, int renderType) {
        if (frameW <= 0 || frameH <= 0) {
            throw new IllegalArgumentException("Frame width and height is unavailable");
        } else {
            setEGLContextClientVersion(2);
            setRenderer(this);
            setRenderMode(RENDERMODE_WHEN_DIRTY);
            setDebugFlags(DEBUG_CHECK_GL_ERROR | DEBUG_LOG_GL_CALLS);
            switch (renderType) {
                case COMMON:
                    render = new RenderCommon(this, frameW, frameH);
                    break;
                case BEAUTY:
                    render = new RenderBeauty(this, frameW, frameH);
                    break;
                case DEPTH:
                    render = new RenderDepth(this, frameW, frameH);
                    break;
                default:
                    throw new IllegalArgumentException("Not support render type: " + renderType);
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
