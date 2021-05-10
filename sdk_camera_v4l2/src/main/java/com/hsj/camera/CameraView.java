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

    private IRender render;

    public CameraView(Context context) {
        this(context, null);
    }

    public CameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setZOrderOnTop(true);
        setZOrderMediaOverlay(true);
    }

    public void setRender(IRender render) {
        if (render != null) {
            this.render = render;
            setEGLContextClientVersion(2);
            setRenderer(this);
            setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        }
    }

    @Override
    public void onResume() {
        if (render != null) render.onResume();
        super.onResume();
    }

    @Override
    public void onPause() {
        if (render != null) render.onPause();
        super.onPause();
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
