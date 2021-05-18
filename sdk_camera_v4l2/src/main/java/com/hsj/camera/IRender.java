package com.hsj.camera;

import android.opengl.GLSurfaceView;
import java.nio.Buffer;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:IRender
 * @Desc:
 */
public interface IRender extends GLSurfaceView.Renderer {
     void onRender(boolean isResume);
     void updatePreview(Buffer frame);
}
