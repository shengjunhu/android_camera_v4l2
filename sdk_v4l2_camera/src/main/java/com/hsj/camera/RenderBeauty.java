package com.hsj.camera;

import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:RenderBeauty
 * @Desc:
 */
final class RenderBeauty implements IRender {

    private static final String TAG = "RenderBeauty";

    /*
     * 顶点坐标
     */
    private static final float VERTEX_BUFFER[] = {
            -1.0f, 1.0f,  //top left
            -1.0f, -1.0f,  //bottom left
            1.0f, 1.0f,  //top right
            1.0f, -1.0f,  //bottom left
    };

    /*
     * 纹理坐标: 旋转90°, 再竖直镜像
     * Camera: 后置Sensor->Rotate90°, 前置使用->Mirror
     */
    private static final float TEXTURE_BUFFER[] = {
            0.0f, 0.0f,  //top left
            1.0f, 0.0f,  //bottom left
            0.0f, 1.0f,  //top right
            1.0f, 1.0f,  //bottom right
    };

    private int program, vPosition, vTexCoord, vTexture;
    //顶点坐标
    private FloatBuffer vertexBuffer;
    //纹理坐标
    private FloatBuffer textureBuffer;
    //纹理
    private int[] textures = new int[1];

    public RenderBeauty(GLSurfaceView glSurfaceView) {
        this.glSurfaceView = glSurfaceView;
        //创建顶点坐标
        ByteBuffer bb1 = ByteBuffer.allocateDirect(32);
        bb1.order(ByteOrder.nativeOrder());
        this.vertexBuffer = bb1.asFloatBuffer();
        this.vertexBuffer.put(VERTEX_BUFFER);
        this.vertexBuffer.position(0);
        //创建纹理坐标
        ByteBuffer bb2 = ByteBuffer.allocateDirect(32);
        bb2.order(ByteOrder.nativeOrder());
        this.textureBuffer = bb2.asFloatBuffer();
        this.textureBuffer.put(TEXTURE_BUFFER);
        this.textureBuffer.position(0);
    }

//==================================================================================================

    //GLSurfaceView
    private GLSurfaceView glSurfaceView;
    private SurfaceTexture surfaceTexture;
    private ISurfaceCallback callback;

    @Override
    public void setSurfaceCallback(ISurfaceCallback callback) {
        this.callback = callback;
        if (this.callback != null) {
            if (this.surfaceTexture != null) {
                this.callback.onSurface(new Surface(this.surfaceTexture));
            } else if (this.textures[0] != 0) {
                this.surfaceTexture = new SurfaceTexture(this.textures[0]);
                this.surfaceTexture.setOnFrameAvailableListener(surfaceTexture -> glSurfaceView.requestRender());
                this.callback.onSurface(new Surface(this.surfaceTexture));
            }
        }
    }

    @Override
    public void onRender(boolean isResume) {
        if (isResume) {
            this.glSurfaceView.onResume();
        } else {
            if (this.callback != null) {
                this.callback.onSurface(null);
            }
            this.glSurfaceView.onPause();
            this.textures[0] = 0;
            if (this.surfaceTexture != null) {
                this.surfaceTexture.release();
                this.surfaceTexture = null;
            }
        }
    }

//==================================================================================================

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        //1-create gl condition
        createGlCondition();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        //2-reset view port
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        //3-render frame
        if (this.textures[0] != 0) {
            this.surfaceTexture.updateTexImage();
            renderFrame();
        }
    }

//==================================================================================================

    private String getShader(AssetManager assets, String fileName) {
        if (assets == null || TextUtils.isEmpty(fileName)) return null;
        StringBuilder content = new StringBuilder();
        InputStream is = null;
        try {
            int ch;
            byte[] buffer = new byte[1024];
            is = assets.open(fileName);
            while (-1 != (ch = is.read(buffer))) {
                content.append(new String(buffer, 0, ch));
            }
        } catch (Exception e) {
            e.printStackTrace();
            content.delete(0, content.length());
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return content.toString();
    }

    private int loadShader(int shaderType, String shaderSource) {
        int shader = GLES20.glCreateShader(shaderType);
        if (shader > GLES20.GL_NONE) {
            GLES20.glShaderSource(shader, shaderSource);
            GLES20.glCompileShader(shader);
            int[] compiled = new int[1];
            GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
            if (compiled[0] == GLES20.GL_FALSE) {
                Log.e(TAG, "GLES20 Error: " + GLES20.glGetShaderInfoLog(shader));
                GLES20.glDeleteShader(shader);
                shader = GLES20.GL_NONE;
            }
        }
        return shader;
    }

    private void checkGlError(String action) {
        int error = GLES20.glGetError();
        if (GLES20.GL_NO_ERROR != error) {
            Log.e(TAG, action + " glError:" + error);
        }
    }

    private void createGlCondition() {
        AssetManager assets = glSurfaceView.getContext().getResources().getAssets();
        //1.1-加载shader
        String shaderVertex = getShader(assets, "beauty_vertex.glsl");
        int vertexId = loadShader(GLES20.GL_VERTEX_SHADER, shaderVertex);
        checkGlError("loadShaderVertex");
        if (GLES20.GL_NONE == vertexId) return;
        String shaderFragment = getShader(assets, "beauty_fragment.glsl");
        int fragmentId = loadShader(GLES20.GL_FRAGMENT_SHADER, shaderFragment);
        checkGlError("loadShaderFragment");
        if (GLES20.GL_NONE == fragmentId) return;
        //1.2-创建program
        program = GLES20.glCreateProgram();
        checkGlError("glCreateProgram");
        if (GLES20.GL_NONE == program) return;
        //1.3-添加program和shader
        GLES20.glAttachShader(program, vertexId);
        checkGlError("glAttachShaderVertex");
        GLES20.glAttachShader(program, fragmentId);
        checkGlError("glAttachShaderFragment");
        //1.4-release
        GLES20.glDeleteShader(vertexId);
        checkGlError("glDeleteShaderVertex");
        GLES20.glDeleteShader(fragmentId);
        checkGlError("glDeleteShaderFragment");
        //1.5-link program
        GLES20.glLinkProgram(program);
        checkGlError("glLinkProgram");
        //1.6-checkLink
        int[] linkStatus = new int[1];
        GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
        if (linkStatus[0] == GLES20.GL_FALSE) {
            Log.e(TAG, "GLES20 Error: glLinkProgram");
            Log.e(TAG, GLES20.glGetProgramInfoLog(program));
            GLES20.glDeleteProgram(program);
            program = GLES20.GL_NONE;
            return;
        }
        //1.7-获取属性位置值
        vPosition = GLES20.glGetAttribLocation(program, "vPosition");
        vTexCoord = GLES20.glGetAttribLocation(program, "vTexCoord");
        vTexture = GLES20.glGetUniformLocation(program, "vTexture");
        //1.8-创建纹理
        GLES20.glGenTextures(1, textures, 0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textures[0]);
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
        checkGlError("createTexture");
        //1.9-create surfaceTexture
        if (this.callback != null) {
            this.surfaceTexture = new SurfaceTexture(this.textures[0]);
            this.surfaceTexture.setOnFrameAvailableListener(surfaceTexture -> glSurfaceView.requestRender());
            this.callback.onSurface(new Surface(this.surfaceTexture));
        }
    }

    private void renderFrame() {
        //3.1-清空画布
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        //3.2-使用句柄
        GLES20.glUseProgram(program);
        //3.3-绑定默认纹理
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textures[0]);
        GLES20.glUniform1i(vTexture, 0);
        //3.4-启用顶点坐标和纹理坐标进行绘制
        GLES20.glEnableVertexAttribArray(vPosition);
        GLES20.glVertexAttribPointer(vPosition, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer);
        GLES20.glEnableVertexAttribArray(vTexCoord);
        GLES20.glVertexAttribPointer(vTexCoord, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);
        //3.5-绘制
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        //3.6-禁用顶点属性数组
        GLES20.glDisableVertexAttribArray(vPosition);
        GLES20.glDisableVertexAttribArray(vTexCoord);
        //3.7-解绑
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_NONE);
    }

}
