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
import javax.microedition.khronos.opengles.GL10;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:CameraRender
 * @Desc:
 */
final class RenderRGB implements IRender {

    private static final String TAG = "RenderRGB";

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
            "attribute mediump vec2 vPosition;\n"
                    + "attribute mediump vec2 vTexCoord;\n"
                    + "varying mediump vec2 texCoord;\n"
                    + "uniform mediump int uRotation;\n"
                    + "uniform mediump float uScaleW;\n"
                    + "uniform mediump float uScaleH;\n"
                    + "\n"
                    + "void main(){\n"
                    + "    vec2 rotPos = vPosition;\n"
                    + "    if(uRotation == 1){\n"
                    + "        rotPos = vPosition * mat2(0, -1, 1, 0);\n"
                    + "    }else if(uRotation == 2){\n"
                    + "        rotPos = vPosition * mat2(-1, 0, 0, -1);\n"
                    + "    }else if(uRotation == 3){\n"
                    + "        rotPos = vPosition * mat2(0, 1, -1, 0);\n"
                    + "    }\n"
                    + "    mat2 scaleMatrix = mat2(uScaleW, 0, 0, uScaleH);\n"
                    + "    gl_Position = vec4(rotPos * scaleMatrix, 1.0, 1.0);\n"
                    + "    texCoord = vTexCoord;\n"
                    + "}\n";

    private static final String SHADER_FRAGMENT =
            "#extension GL_OES_EGL_image_external : require\n"
                    + "varying mediump vec2 texCoord;\n"
                    + "uniform mediump sampler2D texY;\n"
                    + "uniform mediump sampler2D texUV;\n"
                    + "uniform mediump int uRotation;\n"
                    + "uniform mediump int uMirror;\n"
                    + "\n"
                    + "mediump vec4 getBaseColor(in mediump vec2 coord){\n"
                    + "    mediump float y, u, v, r, g, b;\n"
                    + "    mediump vec4 uv;\n"
                    + "    y = texture2D(texY, coord).r - 0.0625;\n"
                    + "    uv = texture2D(texUV, coord);\n"
                    + "    u = uv.r - 0.5;\n"
                    + "    v = uv.a - 0.5;\n"
                    + "    r = y + 1.13983*v;\n"
                    + "    g = y - 0.39465*u - 0.58060*v;\n"
                    + "    b = y + 2.03211*u;\n"
                    + "    return vec4(r, g, b, 1.0);\n"
                    + "}\n"
                    + "\n"
                    + "mediump vec2 mirrorUV(){\n"
                    + "    mediump vec2 mirrorCoord = texCoord;\n"
                    + "    if(uMirror == 1){\n"
                    + "        if(uRotation == 1 || uRotation == 3){\n"
                    + "            mirrorCoord.y = 1.0 - mirrorCoord.y;\n"
                    + "        }else{\n"
                    + "            mirrorCoord.x = 1.0 - mirrorCoord.x;\n"
                    + "        }\n"
                    + "    }\n"
                    + "    return mirrorCoord;\n"
                    + "}\n"
                    + "\n"
                    + "void main(){\n"
                    + "    mediump vec2 mirrorTC = mirrorUV();\n"
                    + "    gl_FragColor = getBaseColor(mirrorTC);\n"
                    + "}\n";

    //OpenGL句柄
    private int program;
    //Shader参数
    private int vPosition, vTexCoord;
    private int uScaleW, uScaleH;
    private int uRotation, uMirror;
    //顶点缓冲坐标
    private FloatBuffer vertexArray;
    //纹理缓冲坐标
    private FloatBuffer textureArray;
    //渲染Y和UV
    private int[] textures = new int[2];
    //加载Y和UV
    private int[] texUniLocation = new int[2];
    //GLSurfaceView 和 Frame 宽高比
    private float scaleW = 1.0f, scaleH = 1.0f;
    private GLSurfaceView glSurfaceView;
    //Frame宽高
    private int frameW, frameH, frameWH;
    //Frame
    private Buffer frame;

    public RenderRGB(GLSurfaceView glSurfaceView, int frameW, int frameH) {
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
    public synchronized void onRender(boolean isResume) {
        if (isResume){
            this.glSurfaceView.onResume();
        }else {
            this.glSurfaceView.onPause();
        }
        this.isRender = isResume;
    }

    @Override
    public synchronized void updatePreview(Buffer frame) {
        if (this.isRender){
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
        //2-update view port
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        //3-Render frame
        renderFrame();
    }

//==================================================================================================

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
        int error;
        if (GLES20.GL_NO_ERROR != (error = GLES20.glGetError())) {
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
        vPosition = GLES20.glGetAttribLocation(program, "vPosition");
        vTexCoord = GLES20.glGetAttribLocation(program, "vTexCoord");
        uScaleW = GLES20.glGetUniformLocation(program, "uScaleW");
        uScaleH = GLES20.glGetUniformLocation(program, "uScaleH");
        uMirror = GLES20.glGetUniformLocation(program, "uMirror");
        uRotation = GLES20.glGetUniformLocation(program, "uRotation");
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
        //3.2-use program
        GLES20.glUseProgram(program);
        //3.3-设置参数
        GLES20.glUniform1f(uScaleW, scaleW);
        GLES20.glUniform1f(uScaleH, scaleH);
        GLES20.glUniform1i(uRotation, 3);
        GLES20.glUniform1i(uMirror, 1);

        //3.4.1-使用texture_Y
        frame.limit(frameWH);
        frame.position(0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
        GLES20.glUniform1i(texUniLocation[0], 0);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE,
                frameW, frameH, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, frame);
        //3.4.2-使用texUV
        frame.position(frameWH);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[1]);
        GLES20.glUniform1i(texUniLocation[1], 1);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE_ALPHA,
                frameW >> 1, frameH >> 1, 0,
                GLES20.GL_LUMINANCE_ALPHA, GLES20.GL_UNSIGNED_BYTE, frame);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        //3.5.1-设置渲染的坐标
        GLES20.glEnableVertexAttribArray(vPosition);
        GLES20.glVertexAttribPointer(vPosition, 2, GLES20.GL_FLOAT, false, 8, vertexArray);
        //3.5.2-设置渲染的纹理属性值
        GLES20.glEnableVertexAttribArray(vTexCoord);
        GLES20.glVertexAttribPointer(vTexCoord, 2, GLES20.GL_FLOAT, false, 8, textureArray);

        //3.6-完成
        frame = null;
        GLES20.glFinish();
        //3.7-禁用顶点属性数组
        GLES20.glDisableVertexAttribArray(vPosition);
        GLES20.glDisableVertexAttribArray(vTexCoord);
        //3.8-解绑
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        Log.e(TAG,"renderTime="+(System.currentTimeMillis()-start));
    }

}
