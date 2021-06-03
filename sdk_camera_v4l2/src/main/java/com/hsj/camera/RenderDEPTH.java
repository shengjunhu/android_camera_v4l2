package com.hsj.camera;

import android.graphics.Color;
import android.opengl.GLES10;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.Log;
import android.view.Surface;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:RenderDEPTH
 * @Desc:
 */
public final class RenderDEPTH implements IRender {

    private static final String TAG = "RenderDEPTH";

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
            "attribute vec4 vPosition;\n"
                    + "uniform mat4 vMatrix;\n"
                    + "attribute vec2 vTexCoord;\n"
                    + "varying vec2 texCoord;\n"
                    + "void main() {\n"
                    + "   texCoord = vTexCoord;\n"
                    + "   gl_Position = vMatrix*vPosition;\n"
                    + "}\n";

    private static final String SHADER_FRAGMENT =
            "#extension GL_OES_EGL_image_external : require\n"
                    + "precision mediump float;\n"
                    + "uniform sampler2D texDepth;\n"
                    + "varying vec2 texCoord;\n"
                    + "void main() {\n"
                    + "   float depth = texture2D(texDepth, texCoord).r;\n"
                    + "   gl_FragColor = vec4(depth, depth, depth, 1.0);\n"
                    + "}\n";

    private int program;
    private int vMatrix;
    private int vPosition;
    private int vTexCoord;
    //加载YUYV
    private int texDepth;
    //顶点缓冲坐标
    private FloatBuffer vertexBuffer;
    //纹理缓冲坐标
    private FloatBuffer textureBuffer;
    //matrix
    private float[] matrix = new float[16];
    //渲染Y和UV
    private int[] textures = new int[1];
    //Frame宽高
    private int frameW, frameH;

    public RenderDEPTH(GLSurfaceView glSurfaceView, int frameW, int frameH) {
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

    //Frame
    private ByteBuffer frame;
    //是否渲染
    private volatile boolean isRender;
    //GLSurfaceView
    private GLSurfaceView glSurfaceView;

    @Override
    public Surface getSurface() {
        return null;
    }

    @Override
    public synchronized void onRender(boolean isResume) {
        if (isResume) {
            this.glSurfaceView.onResume();
            this.isRender = true;
        } else {
            this.isRender = false;
            this.glSurfaceView.onPause();
        }
    }

    @Override
    public synchronized void updatePreview(ByteBuffer frame) {
        if (this.isRender) {
            this.frame = frame;
            this.glSurfaceView.requestRender();
        }
    }

//==================================================================================================

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        //1-Create OpenGL condition
        createGlCondition();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        //2.1-update view port
        setShowMatrix(matrix, frameH, frameW, width, height);
        //2.2-x轴做镜像
        Matrix.scaleM(matrix, 0, -1f, 1f, 1f);
        //2.3-读取到数据是后置旋转270
        Matrix.rotateM(matrix, 0, 270f, 0f, 0f, 1f);
        //2.4-重置坐标
        GLES20.glViewport(0, 0, width, height);
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
        int shader = GLES20.glCreateShader(shaderType);
        if (shader > GLES20.GL_NONE) {
            GLES20.glShaderSource(shader, shaderSource);
            GLES20.glCompileShader(shader);
            int[] compiled = new int[1];
            GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
            if (compiled[0] == GLES20.GL_FALSE) {
                Log.e(TAG, "GLES20 Error: " + GLES20.glGetShaderInfoLog(shader));
                GLES20.glDeleteShader(shader);
                shader = 0;
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
        //1.1-创建纹理
        createTexture();
        //1.2-加载shader
        int vertexId = loadShader(GLES20.GL_VERTEX_SHADER, SHADER_VERTEX);
        int fragmentId = loadShader(GLES20.GL_FRAGMENT_SHADER, SHADER_FRAGMENT);
        //1.3-创建program
        program = GLES20.glCreateProgram();
        if (program == GLES20.GL_NONE || vertexId == GLES20.GL_NONE || fragmentId == GLES20.GL_NONE) {
            Log.e(TAG, "GL: program=" + program);
            Log.e(TAG, "GL: vertex=" + vertexId);
            Log.e(TAG, "GL: fragment=" + fragmentId);
        } else {
            //1.4-添加program和shader
            GLES20.glAttachShader(program, vertexId);
            GLES20.glAttachShader(program, fragmentId);
            //1.5-link program
            GLES20.glLinkProgram(program);
            //1.6-release
            GLES20.glDeleteShader(vertexId);
            GLES20.glDeleteShader(fragmentId);
        }
        //1.6-获取属性值
        vMatrix = GLES20.glGetUniformLocation(program, "vMatrix");
        vPosition = GLES20.glGetAttribLocation(program, "vPosition");
        vTexCoord = GLES20.glGetAttribLocation(program, "vTexCoord");
        //1.6.2-获取shader里depth对应的纹理uniform变量的地址,这个在后面会被设置到Texture上
        texDepth = GLES20.glGetUniformLocation(program, "texDepth");
    }

    private void createTexture() {
        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
        //设置白色
        GLES10.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        //启用GL_TEXTURE_2D
        GLES20.glEnable(GLES20.GL_TEXTURE_2D);
        //生成texDepth
        GLES20.glGenTextures(1, textures, 0);
        //绑定texDepth
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
    }

    private synchronized void renderFrame() {
        //3.1-清空画布
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
        if (program == 0 || frame == null) return;
        //3.2-使用program
        GLES20.glUseProgram(program);
        //3.3-设置渲染的坐标
        GLES20.glUniformMatrix4fv(vMatrix, 1, false, matrix, 0);

        //3.4.1-绑定texDepth
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
        //3.4.2-使用texDepth
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE_ALPHA,
                frameW, frameH, 0, GLES20.GL_LUMINANCE_ALPHA, GLES20.GL_UNSIGNED_BYTE, frame);
        GLES20.glUniform1i(texDepth, 0);

        //3.5.1-设置渲染的坐标
        GLES20.glEnableVertexAttribArray(vPosition);
        GLES20.glVertexAttribPointer(vPosition, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer);
        //3.5.2-设置渲染的纹理属性值
        GLES20.glEnableVertexAttribArray(vTexCoord);
        GLES20.glVertexAttribPointer(vTexCoord, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        //3.6-禁用顶点属性数组
        frame = null;
        GLES20.glFinish();
        GLES20.glDisableVertexAttribArray(vPosition);
        GLES20.glDisableVertexAttribArray(vTexCoord);
        //3.7-解绑：4ms
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, GLES20.GL_NONE);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, GLES20.GL_NONE);
    }

}
