package com.hsj.camera;

import android.opengl.GLSurfaceView;
import android.view.Surface;

import java.nio.ByteBuffer;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:IRender
 * @Desc:
 */
public interface IRender extends GLSurfaceView.Renderer {
     Surface getSurface();
     void onRender(boolean isResume);
}
