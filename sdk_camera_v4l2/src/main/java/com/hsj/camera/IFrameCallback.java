package com.hsj.camera;

import java.nio.ByteBuffer;

/**
 * @Author:Hsj
 * @Date:2021/5/11
 * @Class:IFrameCallback
 * @Desc:
 */
public interface IFrameCallback {
    void onFrame(ByteBuffer data);
}
