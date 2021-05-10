package com.hsj.camera;

import android.opengl.GLES20;
import android.util.Log;

import java.io.FileInputStream;
import java.io.IOException;
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
public final class RenderRGB extends IRender {

    private static final String TAG = "RenderRGB";

    //顶点坐标
    private static final float VERTEX_ARRAY[] = {
//            -1.0f, -1.0f,    // 0 bottom left
//            1.0f, -1.0f,     // 1 bottom right
//            -1.0f, 1.0f,     // 2 top left
//            1.0f, 1.0f       // 3 top right
            1f,1f,
            -1f,1f,
            1f,-1f,
            -1f,-1f
    };

    //纹理坐标
    private static final float TEXTURE_ARRAY[] = {
//            0.0f, 1.0f,       // 0 top left
//            1.0f, 1.0f,       // 1 top right
//            0.0f, 0.0f,       // 2 bottom left
//            1.0f, 0.0f        // 3 bottom right
            1f,0f,
            0f,0f,
            1f,1f,
            0f,1f
    };

    private static final String SHADER_VERTEX =
            "attribute vec4 aPosition;\n"
                    + "attribute vec2 aTextureCoord;\n"
                    + "varying vec2 vTextureCoord;\n"
                    + "void main() {\n"
                    + "  gl_Position = aPosition;\n"
                    + "  vTextureCoord = aTextureCoord;\n"
                    + "}\n";

    private static final String SHADER_FRAGMENT =
            "precision mediump float;\n"
                    + "uniform sampler2D Ytex;\n"
                    + "uniform sampler2D UVtex;\n"
                    + "varying vec2 vTextureCoord;\n"
                    + "void main(void) {\n"
                    + "    float r,g,b,y,u,v;\n"
                    + "    y = texture2D(Ytex, vTextureCoord).r;\n"
                    + "    u = texture2D(UVtex,vTextureCoord).r - 0.5;\n"
                    + "    v = texture2D(UVtex,vTextureCoord).a - 0.5;\n"
                    + "    r = y + 1.13983*v;\n"
                    + "    g = y - 0.39465*u - 0.58060*v;\n"
                    + "    b = y + 2.03211*u;\n"
                    + "    gl_FragColor = vec4(r, g, b, 1.0);\n"
                    + "}\n";

    private int positionHandle;
    private int textureHandle;
    //
    private FloatBuffer vertexArray;
    private FloatBuffer textureArray;
    //渲染Y和UV
    private int[] textures = new int[2];
    //加载Y和UV
    private int[] texUniLocation = new int[2];
    //matrix
    private float[] matrix = new float[16];
    //Surface宽高
    private int width, height;

    public RenderRGB() {
        //创建顶点坐标
        vertexArray = createBuffer(VERTEX_ARRAY);
        //创建纹理坐标
        textureArray = createBuffer(TEXTURE_ARRAY);
        //
        frameData = readFile();
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

    public int getTextureId(){
        return textures[0];
    }

    public void onResume() {

    }

    public void onPause() {

    }

//==================================================================================================

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        //1.1-创建纹理
        Log.e(TAG,"onSurfaceCreated-1.1");
        createTexture();
        //1.2-加载shader
        Log.e(TAG,"onSurfaceCreated-1.2");
        int shaderVertex = loadShader(GLES20.GL_VERTEX_SHADER, SHADER_VERTEX);
        int shaderFragment = loadShader(GLES20.GL_FRAGMENT_SHADER, SHADER_FRAGMENT);
        //1.3-创建program
        Log.e(TAG,"onSurfaceCreated-1.3");
        int program = GLES20.glCreateProgram();
        //1.4-绑定program和shader
        Log.e(TAG,"onSurfaceCreated-1.4,"+program);
        if (program != 0) {
            GLES20.glAttachShader(program, shaderVertex);
            GLES20.glAttachShader(program, shaderFragment);
            //1.4.2-link program
            GLES20.glLinkProgram(program);
            GLES20.glUseProgram(program);
        } else {
            Log.e(TAG, "onSurfaceCreated: GLES20.glCreateProgram() failed");
        }
        //1.5.1-获取属性值
        Log.e(TAG,"onSurfaceCreated-1.5");
        positionHandle = GLES20.glGetAttribLocation(program, "aPosition");
        textureHandle = GLES20.glGetAttribLocation(program, "aTextureCoord");
        //1.5.2-获取shader里y和uv对应的纹理uniform变量的地址,这个在后面会被设置到Texture上
        texUniLocation[0] = GLES20.glGetUniformLocation(program, "Ytex");
        texUniLocation[1] = GLES20.glGetUniformLocation(program, "UVtex");
        //1.5.3-设置渲染的坐标
        GLES20.glVertexAttribPointer(positionHandle, 2, GLES20.GL_FLOAT, false, 8, vertexArray);
        GLES20.glEnableVertexAttribArray(positionHandle);
        //1.5.4-设置渲染的纹理属性值
        GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 8, textureArray);
        GLES20.glEnableVertexAttribArray(textureHandle);
        //1.6-使用program
        Log.e(TAG,"onSurfaceCreated-1.6");
        GLES20.glUseProgram(program);
        Log.e(TAG,"onSurfaceCreated-1.6.1");
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        //2-更新坐标
        Log.e(TAG,"onSurfaceChanged");
        GLES20.glViewport(0, 0, width, height);
        this.width = width;
        this.height = height;
        Log.e(TAG,"onSurfaceChanged-2");
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        //3.1-清空画布
        Log.e(TAG,"onDrawFrame-3.1");
        GLES20.glClearColor(0f, 0f, 0f, 0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
        //3.2-绘制
        Log.e(TAG,"onDrawFrame-3.2");
        drawFrame();
    }

//==================================================================================================

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

    private void checkGlError(String action) {
        int error;
        if (GLES20.GL_NO_ERROR != (error = GLES20.glGetError())) {
            Log.e(TAG, action + " glError:" + error);
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

    private void drawFrame() {
        Log.e(TAG,"drawFrame");
        if (frameData == null) return;
        Log.e(TAG,"drawFrame2");

        frameData.limit(width * height);
        frameData.position(0);

        //3.3-使用texture_Y
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
        //这里将之前的纹理信息设置到纹理上，注意这里使用的纹理(x值)要和对应好
        GLES20.glUniform1i(texUniLocation[0], 0);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width, height,
                0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, frameData);

        //3.4-使用texture_UV
        frameData.position(width * height);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[1]);
        GLES20.glUniform1i(texUniLocation[1], 1);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE_ALPHA,
                width >> 1, height >> 1, 0, GLES20.GL_LUMINANCE_ALPHA,
                GLES20.GL_UNSIGNED_BYTE, frameData);

        //3.5-完成
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glFinish();

        //3.6-禁用顶点属性数组
        GLES20.glDisableVertexAttribArray(positionHandle);
        GLES20.glDisableVertexAttribArray(textureHandle);
    }

//==================================================================================================

    private static final String NV12 = "/sdcard/rgb_1280x800.yuv";
    private ByteBuffer frameData;

    private ByteBuffer readFile() {
        ByteBuffer frame = null;
        FileInputStream fis = null;
        FileChannel fc = null;
        try {
            fis = new FileInputStream(NV12);
            fc = fis.getChannel();
            frame = ByteBuffer.allocateDirect(640 * 40 * 15);
            fc.read(frame);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (fc != null) fc.close();
                if (fis != null) fis.close();
            } catch (IOException e2) {
                e2.printStackTrace();
            }
        }
        return frame;
    }

}
