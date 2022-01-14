package com.hsj.camera;

import android.opengl.GLSurfaceView;
import android.view.Surface;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:IRender
 * @Desc:
 */
public interface IRender extends GLSurfaceView.Renderer {
     void onRender(boolean isResume);
     void setSurfaceCallback(ISurfaceCallback callback);
}
