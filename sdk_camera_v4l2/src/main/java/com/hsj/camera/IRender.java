package com.hsj.camera;

import android.opengl.GLSurfaceView;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:IRender
 * @Desc:
 */
public abstract class IRender implements GLSurfaceView.Renderer {

    public abstract void onResume();

    public abstract void onPause();

    public abstract int getTextureId();

}
