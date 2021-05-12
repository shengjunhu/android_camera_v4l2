package com.hsj.camera;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.Log;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.channels.FileChannel;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.opengles.GL10;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:RenderNV12
 * @Desc:
 */
public final class RenderNV12 implements IRender {

    private static final String TAG = "RenderNV12";

    /*
     * 顶点坐标
     * 0 bottom left    (-1.0f, -1.0f)
     * 1 bottom right   (1.0f, -1.0f)
     * 2 top left       (-1.0f, 1.0f)
     * 3 top right      (1.0f, 1.0f)
     */
    private static final float VERTEX_ARRAY[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};

    /*
     * 纹理坐标
     * 0 top left          (0.0f, 1.0f)
     * 1 top right         (1.0f, 1.0f)
     * 2 bottom left       (0.0f, 0.0f)
     * 3 bottom right      (1.0f, 0.0f)
     */
    private static final float TEXTURE_ARRAY[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};

    private static final String SHADER_VERTEX =
            "attribute vec4 vPosition;\n"
                    + "uniform mat4 vMatrix;\n"
                    + "attribute vec2 vTexCoord;\n"
                    + "varying vec2 texCoord;\n"
                    + "void main() {\n"
                    + "   gl_Position = vMatrix*vPosition;\n"
                    + "   texCoord = vTexCoord;\n"
                    + "}\n";

    private static final String SHADER_FRAGMENT =
            "#extension GL_OES_EGL_image_external : require\n"
                    + "precision mediump float;\n"
                    + "uniform sampler2D texY;\n"
                    + "uniform sampler2D texUV;\n"
                    + "varying vec2 texCoord;\n"
                    + "void main() {\n"
                    + "   float r,g,b,y,u,v;\n"
                    + "   y = texture2D(texY, texCoord).r;\n"
                    + "   u = texture2D(texUV,texCoord).r - 0.5;\n"
                    + "   v = texture2D(texUV,texCoord).a - 0.5;\n"
                    + "   r = y + 1.13983*v;\n"
                    + "   g = y - 0.39465*u - 0.58060*v;\n"
                    + "   b = y + 2.03211*u;\n"
                    + "   gl_FragColor = vec4(r, g, b, 1.0);\n"
                    + "}\n";

    private int program;
    private int vMatrix;
    private int vPosition;
    private int vTexCoord;
    //顶点缓冲坐标
    private FloatBuffer vertexArray;
    //纹理缓冲坐标
    private FloatBuffer textureArray;
    //matrix
    private float[] matrix = new float[16];
    //渲染Y和UV
    private int[] textures = new int[2];
    //加载Y和UV
    private int[] texUniLocation = new int[2];
    //GLSurfaceView
    private GLSurfaceView glSurfaceView;
    //Frame宽高
    private int frameW, frameH, frameWH;
    //Frame
    private Buffer frame;

    public RenderNV12(GLSurfaceView glSurfaceView, int frameW, int frameH) {
        this.glSurfaceView = glSurfaceView;
        this.frameW = frameW;
        this.frameH = frameH;
        this.frameWH = frameW * frameH;
        //创建顶点坐标
        vertexArray = createBuffer(VERTEX_ARRAY);
        //创建纹理坐标
        textureArray = createBuffer(TEXTURE_ARRAY);
    }

    private FloatBuffer createBuffer(float[] Array) {
        ByteBuffer bb = ByteBuffer.allocateDirect(Array.length * 4);
        bb.order(ByteOrder.nativeOrder());
        FloatBuffer fb = bb.asFloatBuffer();
        fb.put(Array);
        fb.position(0);
        return fb;
    }

//==================================================================================================

    private volatile boolean isRender;

    @Override
    public synchronized void release(){
        if (program != 0){
            GLES20.glDeleteProgram(program);
            GLES20.glReleaseShaderCompiler();
            frame = null;
            program = 0;
        }
    }

    @Override
    public synchronized void onRender(boolean isResume) {
        if (isResume) {
            this.glSurfaceView.onResume();
        } else {
            this.glSurfaceView.onPause();
        }
        this.isRender = isResume;
    }

    @Override
    public synchronized void updatePreview(Buffer frame) {
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
        if (shader > 0) {
            GLES20.glShaderSource(shader, shaderSource);
            GLES20.glCompileShader(shader);
            int[] compiled = new int[1];
            GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
            if (compiled[0] == 0) {
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
        int shaderVertex = loadShader(GLES20.GL_VERTEX_SHADER, SHADER_VERTEX);
        int shaderFragment = loadShader(GLES20.GL_FRAGMENT_SHADER, SHADER_FRAGMENT);
        //1.3-创建program
        program = GLES20.glCreateProgram();
        if (program == 0 || shaderVertex == 0 || shaderFragment == 0) {
            Log.w(TAG, "onSurfaceCreated: program=" + program);
            Log.w(TAG, "onSurfaceCreated: vertex=" + shaderVertex);
            Log.w(TAG, "onSurfaceCreated: fragment=" + shaderFragment);
        } else {
            //1.4-添加program和shader
            GLES20.glAttachShader(program, shaderVertex);
            GLES20.glAttachShader(program, shaderFragment);
            //1.5-link program
            GLES20.glLinkProgram(program);
        }
        //1.6-获取属性值
        vMatrix = GLES20.glGetUniformLocation(program, "vMatrix");
        vPosition = GLES20.glGetAttribLocation(program, "vPosition");
        vTexCoord = GLES20.glGetAttribLocation(program, "vTexCoord");
        //1.6.2-获取shader里y和uv对应的纹理uniform变量的地址,这个在后面会被设置到Texture上
        texUniLocation[0] = GLES20.glGetUniformLocation(program, "texY");
        texUniLocation[1] = GLES20.glGetUniformLocation(program, "texUV");
    }

    private void createTexture() {
        GLES20.glGenTextures(2, textures, 0);
        //绑定texture_Y
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        //绑定texture_UV
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[1]);
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
        long start = System.currentTimeMillis();
        //3.2-使用program
        GLES20.glUseProgram(program);
        //3.3-设置渲染的坐标
        GLES20.glUniformMatrix4fv(vMatrix, 1, false, matrix, 0);

        Log.d(TAG, "renderTime1=" + (System.currentTimeMillis() - start));
        //3.4.1-使用texture_Y
        frame.limit(frameWH);
        frame.position(0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
        //这里将之前的纹理信息设置到纹理上，注意这里使用的纹理(x值)要和对应好
        GLES20.glUniform1i(texUniLocation[0], 0);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE,
                frameW, frameH, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, frame);
        Log.d(TAG, "renderTime2=" + (System.currentTimeMillis() - start));
        //3.4.2-使用texture_UV
        frame.position(frameWH);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[1]);
        GLES20.glUniform1i(texUniLocation[1], 1);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE_ALPHA,
                frameW >> 1, frameH >> 1, 0,
                GLES20.GL_LUMINANCE_ALPHA, GLES20.GL_UNSIGNED_BYTE, frame);

        //3.5.1-设置渲染的坐标
        GLES20.glEnableVertexAttribArray(vPosition);
        GLES20.glVertexAttribPointer(vPosition, 2, GLES20.GL_FLOAT, false, 8, vertexArray);
        //3.5.2-设置渲染的纹理属性值
        GLES20.glEnableVertexAttribArray(vTexCoord);
        GLES20.glVertexAttribPointer(vTexCoord, 2, GLES20.GL_FLOAT, false, 8, textureArray);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        frame = null;

        //3.6-禁用顶点属性数组
        //GLES20.glFinish();
        GLES20.glDisableVertexAttribArray(vPosition);
        GLES20.glDisableVertexAttribArray(vTexCoord);
        //3.7-解绑：4ms
        //GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        //GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        Log.d(TAG, "renderTime=" + (System.currentTimeMillis() - start));
    }

}
