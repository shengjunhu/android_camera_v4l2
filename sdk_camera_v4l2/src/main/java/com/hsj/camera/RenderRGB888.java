package com.hsj.camera;

import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLES31;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:RenderRGB
 * @Desc:
 */
public final class RenderRGB888 implements IRender {

    private static final String TAG = "RenderRGB888";

    /*
     * 顶点坐标
     * 0 bottom left    (-1.0f, -1.0f)
     * 1 bottom right   (1.0f, -1.0f)
     * 2 top left       (-1.0f, 1.0f)
     * 3 top right      (1.0f, 1.0f)
     */
    private static final float VERTEX_BUFFER[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};

    /*
     * 纹理坐标
     * 0 top left          (0.0f, 1.0f)
     * 1 top right         (1.0f, 1.0f)
     * 2 bottom left       (0.0f, 0.0f)
     * 3 bottom right      (1.0f, 0.0f)
     */
    private static final float TEXTURE_BUFFER[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};

    private static final String SHADER_VERTEX =
            "#version 310 es\n"
                    + "uniform mat4 vMatrix;\n"
                    + "layout(location = 0) in vec4 vPosition;\n"
                    + "layout(location = 1) in vec2 vTexCoord;\n"
                    + "out vec2 texCoord;\n"
                    + "void main() {\n"
                    + "   texCoord = vTexCoord;\n"
                    + "   gl_Position = vMatrix*vPosition;\n"
                    + "}";

    private static final String SHADER_FRAGMENT =
            "#version 310 es\n"
                    + "#extension GL_OES_EGL_image_external : require\n"
                    + "precision mediump float;\n"
                    + "in vec2 texCoord;\n"
                    + "uniform samplerExternalOES vTexture;\n"
                    + "out vec4 fragColor;\n"
                    + "void main() {\n"
                    + "   fragColor = texture(vTexture,texCoord);\n"
                    + "}";

    private int program;
    private int vMatrix;
    private static final int vPosition = 0;
    private static final int vTexCoord = 1;
    //顶点缓冲坐标
    private FloatBuffer vertexBuffer;
    //纹理缓冲坐标
    private FloatBuffer textureBuffer;
    //matrix
    private float[] matrix = new float[16];
    //加载RGB
    private int[] texUniLocation = new int[1];
    //渲染RGB24
    private int[] textures = new int[1];
    //预览
    private SurfaceTexture surfaceTexture;
    //Frame宽高
    private int frameW, frameH;

    public RenderRGB888(GLSurfaceView glSurfaceView, int frameW, int frameH) {
        this.glSurfaceView = glSurfaceView;
        this.frameW = frameW;
        this.frameH = frameH;
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

    @Override
    public synchronized void onRender(boolean isResume) {
        if (isResume) {
            this.glSurfaceView.onResume();
        } else {
            if (this.surfaceTexture != null) {
                this.surfaceTexture.release();
            }
            this.textures[0] = 0;
            this.glSurfaceView.onPause();
        }
    }

    @Override
    public synchronized void updatePreview(ByteBuffer frame) {

    }

//==================================================================================================

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        //1-Create OpenGL condition
        createGlCondition();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        //2.1-重置坐标
        GLES31.glViewport(0, 0, width, height);
        //2.2-update view port
        setShowMatrix(matrix, frameH, frameW, width, height);
        //2.3-x轴做镜像
        Matrix.scaleM(matrix, 0, -1f, 1f, 1f);
        //2.4-读取到数据是后置旋转270
        Matrix.rotateM(matrix, 0, 270f, 0f, 0f, 1f);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        //3-Render frame
        renderFrame();
    }

//==================================================================================================

    private void setShowMatrix(float[] matrix, int imgW, int imgH, int viewW, int viewH) {
        if (imgW > 0 && imgH > 0 && viewW > 0 && viewH > 0) {
            float whImg = 1.0f * imgW / imgH;
            float whPreview = 1.0f * viewW / viewH;
            float[] projection = new float[16];
            if (whImg > whPreview) {
                Matrix.orthoM(projection, 0, -whPreview / whImg, whPreview / whImg, -1, 1, 1, 3);
            } else {
                Matrix.orthoM(projection, 0, -1, 1, -whImg / whPreview, whImg / whPreview, 1, 3);
            }
            float[] camera = new float[16];
            Matrix.setLookAtM(camera, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);
            Matrix.multiplyMM(matrix, 0, projection, 0, camera, 0);
        }
    }

    private int loadShader(int shaderType, String shaderSource) {
        int shader = GLES31.glCreateShader(shaderType);
        if (shader > GLES31.GL_NONE) {
            GLES31.glShaderSource(shader, shaderSource);
            GLES31.glCompileShader(shader);
            int[] compiled = new int[1];
            GLES31.glGetShaderiv(shader, GLES31.GL_COMPILE_STATUS, compiled, 0);
            if (compiled[0] == GLES31.GL_FALSE) {
                Log.e(TAG, "GLES31 Error: " + GLES31.glGetShaderInfoLog(shader));
                GLES31.glDeleteShader(shader);
                shader = GLES31.GL_NONE;
            }
        }
        return shader;
    }

    private void checkGlError(String action) {
        int error = GLES31.glGetError();
        if (GLES31.GL_NO_ERROR != error) {
            Log.e(TAG, action + " glError:" + error);
        }
    }

    private void createGlCondition() {
        //1.1-加载shader
        int vertexId = loadShader(GLES31.GL_VERTEX_SHADER, SHADER_VERTEX);
        checkGlError("loadShaderVertex");
        if (GLES31.GL_NONE == vertexId) return;
        int fragmentId = loadShader(GLES31.GL_FRAGMENT_SHADER, SHADER_FRAGMENT);
        checkGlError("loadShaderFragment");
        if (GLES31.GL_NONE == fragmentId) return;
        //1.2-创建program
        program = GLES31.glCreateProgram();
        checkGlError("glCreateProgram");
        if (GLES31.GL_NONE == program) return;
        //1.3-添加program和shader
        GLES31.glAttachShader(program, vertexId);
        checkGlError("glAttachShaderVertex");
        GLES31.glAttachShader(program, fragmentId);
        checkGlError("glAttachShaderFragment");
        //1.4-release
        GLES31.glDeleteShader(vertexId);
        checkGlError("glDeleteShaderVertex");
        GLES31.glDeleteShader(fragmentId);
        checkGlError("glDeleteShaderFragment");
        //1.5-link program
        GLES31.glLinkProgram(program);
        checkGlError("glLinkProgram");
        //1.6-checkLink
        int[] linkStatus = new int[1];
        GLES31.glGetProgramiv(program, GLES31.GL_LINK_STATUS, linkStatus, 0);
        if (linkStatus[0] == GLES31.GL_FALSE) {
            Log.e(TAG, "GLES31 Error: glLinkProgram");
            Log.e(TAG, GLES31.glGetProgramInfoLog(program));
            GLES31.glDeleteProgram(program);
            program = GLES31.GL_NONE;
            return;
        }
        //1.7-获取属性位置值
        vMatrix = GLES31.glGetUniformLocation(program, "vMatrix");
        texUniLocation[0] = GLES31.glGetUniformLocation(program, "vTexture");
        //1.8-创建纹理
        GLES31.glGenTextures(textures.length, textures, 0);
        GLES31.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textures[0]);
        GLES31.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        GLES31.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        GLES31.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        GLES31.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
        this.surfaceTexture = new SurfaceTexture(textures[0]);
        this.surfaceTexture.setOnFrameAvailableListener(surfaceTexture -> glSurfaceView.requestRender());
        checkGlError("createTexture");
    }

    private synchronized void renderFrame() {
        long start = System.currentTimeMillis();
        //清空画布
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
        //使用句柄
        GLES20.glUseProgram(program);
        //扩展数据-1
        GLES31.glUniformMatrix4fv(vMatrix, 1, false, matrix, 0);
        //绑定默认纹理
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textures[0]);
        GLES31.glUniform1i(texUniLocation[0], 0);
        //启用顶点坐标和纹理坐标进行绘制
        GLES31.glEnableVertexAttribArray(vPosition);
        GLES31.glVertexAttribPointer(vPosition, 2, GLES31.GL_FLOAT, false, 8, vertexBuffer);
        GLES31.glEnableVertexAttribArray(vTexCoord);
        GLES31.glVertexAttribPointer(vTexCoord, 2, GLES31.GL_FLOAT, false, 8, textureBuffer);
        //绘制
        GLES31.glDrawArrays(GLES31.GL_TRIANGLE_STRIP, 0, 4);
        //禁用顶点属性数组
        GLES31.glDisableVertexAttribArray(vPosition);
        GLES31.glDisableVertexAttribArray(vTexCoord);
        //解绑
        GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, GLES31.GL_NONE);
        Log.d(TAG, "renderTime=" + (System.currentTimeMillis() - start));
    }

}
