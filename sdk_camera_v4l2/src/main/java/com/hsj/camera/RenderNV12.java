package com.hsj.camera;

import android.media.MediaCodec;
import android.opengl.GLES31;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.Log;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
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
            "#version 310 es\n" +
                    "uniform mat4 vMatrix;\n" +
                    "layout(location = 0) in vec4 vPosition;\n" +
                    "layout(location = 1) in vec2 vTexCoord;\n" +
                    "out vec2 texCoord;\n" +
                    "void main() {\n" +
                    "   texCoord = vTexCoord;\n" +
                    "   gl_Position = vMatrix*vPosition;\n" +
                    "}";

    private static final String SHADER_FRAGMENT =
            "#version 310 es\n" +
                    "precision mediump float;\n" +
                    "precision mediump sampler2D;\n" +
                    "const mat3 convert = mat3(1.0,1.0,1.0,0.0,-0.39465,1.53211,1.13983,-0.58060,0.0);\n" +
                    "in vec2 texCoord;\n" +
                    "uniform sampler2D texY;\n" +
                    "uniform sampler2D texUV;\n" +
                    "out vec4 fragColor;\n" +
                    "void main() {\n" +
                    "   vec3 yuv;\n" +
                    "   yuv.x = texture(texY, texCoord).r + 0.0625;\n" +
                    "   yuv.y = texture(texUV,texCoord).r - 0.5;\n" +
                    "   yuv.z = texture(texUV,texCoord).a - 0.5;\n" +
                    "   fragColor = vec4(convert * yuv, 1.0);\n" +
                    "}";

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
    //加载Y和UV
    private int[] texUniLocation = {0, 1};
    //渲染Y和UV
    private int[] textures = new int[2];
    //PBO
    private int[] pboY = new int[1];
    private int[] pboUV = new int[1];
    //Frame宽高
    private int frameW, frameH;
    private int frameW2, frameH2;
    private int frameSize, frameSize2;

    public RenderNV12(GLSurfaceView glSurfaceView, int frameW, int frameH) {
        this.glSurfaceView = glSurfaceView;
        this.frameW = frameW;
        this.frameH = frameH;
        this.frameW2 = frameW >> 1;
        this.frameH2 = frameH >> 1;
        this.frameSize = frameW * frameH;
        this.frameSize2 = frameSize >> 1;
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

    private byte[] data;
    //Frame
    private ByteBuffer frame;
    //是否渲染
    private volatile boolean isRender;
    //GLSurfaceView
    private GLSurfaceView glSurfaceView;

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
            frame.clear();
            this.frame = frame;
            if (data==null) data = new byte[frame.capacity()];
            frame.get(data);
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
        texUniLocation[0] = GLES31.glGetUniformLocation(program, "texY");
        texUniLocation[1] = GLES31.glGetUniformLocation(program, "texUV");
        //1.8-创建纹理
        createTexture();
        checkGlError("createTexture");
        //1.9-创建PBO Y
        GLES31.glGenBuffers(pboY.length, pboY, 0);
        GLES31.glBindBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER, pboY[0]);
        GLES31.glBufferData(GLES31.GL_PIXEL_UNPACK_BUFFER, frameSize, null, GLES31.GL_STREAM_DRAW);
        GLES31.glBindBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER, GLES31.GL_NONE);
        //1.9-创建PBO UV
        GLES31.glGenBuffers(pboUV.length, pboUV, 0);
        GLES31.glBindBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER, pboUV[0]);
        GLES31.glBufferData(GLES31.GL_PIXEL_UNPACK_BUFFER, frameSize2, null, GLES31.GL_STREAM_DRAW);
        GLES31.glBindBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER, GLES31.GL_NONE);
    }

    private void createTexture() {
        //生成纹理
        GLES31.glGenTextures(textures.length, textures, 0);
        //绑定texture_Y
        GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, textures[0]);
        GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D, GLES31.GL_TEXTURE_WRAP_S, GLES31.GL_REPEAT);
        GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D, GLES31.GL_TEXTURE_WRAP_T, GLES31.GL_REPEAT);
        GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D, GLES31.GL_TEXTURE_MIN_FILTER, GLES31.GL_LINEAR);
        GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D, GLES31.GL_TEXTURE_MAG_FILTER, GLES31.GL_LINEAR);
        GLES31.glTexImage2D(GLES31.GL_TEXTURE_2D, 0, GLES31.GL_LUMINANCE,
                frameW, frameH, 0, GLES31.GL_LUMINANCE, GLES31.GL_UNSIGNED_BYTE, null);
        //绑定texture_UV
        GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, textures[1]);
        GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D, GLES31.GL_TEXTURE_WRAP_S, GLES31.GL_REPEAT);
        GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D, GLES31.GL_TEXTURE_WRAP_T, GLES31.GL_REPEAT);
        GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D, GLES31.GL_TEXTURE_MIN_FILTER, GLES31.GL_LINEAR);
        GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D, GLES31.GL_TEXTURE_MAG_FILTER, GLES31.GL_LINEAR);
        GLES31.glTexImage2D(GLES31.GL_TEXTURE_2D, 0, GLES31.GL_LUMINANCE_ALPHA, frameW2,
                frameH2, 0, GLES31.GL_LUMINANCE_ALPHA, GLES31.GL_UNSIGNED_BYTE, null);
    }

    private synchronized void renderFrame() {
        if (program == 0 || frame == null) return;
        long start = System.currentTimeMillis();

        //3.1-清空画布
        GLES31.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES31.glClear(GLES31.GL_COLOR_BUFFER_BIT);
        //3.2-使用program
        GLES31.glUseProgram(program);
        //3.3-设置渲染的坐标
        GLES31.glUniformMatrix4fv(vMatrix, 1, false, matrix, 0);

        //3.4-PBO Y
        //frame.limit(frameSize);
        //frame.position(0);
        GLES31.glActiveTexture(GLES31.GL_TEXTURE0);
        GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, textures[0]);
        GLES31.glUniform1i(texUniLocation[0], 0);
        //TODO==========================StartY
        GLES31.glBindBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER, pboY[0]);
        GLES31.glTexImage2D(GLES31.GL_TEXTURE_2D, 0, GLES31.GL_LUMINANCE,
                frameW, frameH, 0, GLES31.GL_LUMINANCE, GLES31.GL_UNSIGNED_BYTE, null);
        GLES31.glBufferData(GLES31.GL_PIXEL_UNPACK_BUFFER, frameSize, null, GLES31.GL_STREAM_DRAW);
        ByteBuffer buffer = (ByteBuffer) GLES31.glMapBufferRange(GLES31.GL_PIXEL_UNPACK_BUFFER, 0,
                frameSize, GLES31.GL_MAP_WRITE_BIT | GLES31.GL_MAP_INVALIDATE_BUFFER_BIT);
        if (buffer != null) {
            buffer.put(data,0, frameSize);
            GLES31.glUnmapBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER);
        }else {
            checkGlError("glMapBufferRange");
        }
        GLES31.glBindBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER, GLES31.GL_NONE);
        //TODO==========================EndY

        //3.5-PBO UV
        //frame.position(frameSize);
        GLES31.glActiveTexture(GLES31.GL_TEXTURE1);
        GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, textures[1]);
        GLES31.glUniform1i(texUniLocation[1], 1);
        //TODO==========================StartUV
        GLES31.glBindBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER, pboUV[0]);
        GLES31.glTexImage2D(GLES31.GL_TEXTURE_2D, 0, GLES31.GL_LUMINANCE_ALPHA, frameW2,
                frameH2, 0, GLES31.GL_LUMINANCE_ALPHA, GLES31.GL_UNSIGNED_BYTE, null);
        GLES31.glBufferData(GLES31.GL_PIXEL_UNPACK_BUFFER, frameSize2, null, GLES31.GL_STREAM_DRAW);
        ByteBuffer buffer2 = (ByteBuffer) GLES31.glMapBufferRange(GLES31.GL_PIXEL_UNPACK_BUFFER, 0,
                frameSize2, GLES31.GL_MAP_WRITE_BIT | GLES31.GL_MAP_INVALIDATE_BUFFER_BIT);
        if (buffer2 != null) {
            buffer2.put(data,frameSize,frameSize2);
            GLES31.glUnmapBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER);
        }else {
            checkGlError("glMapBufferRange");
        }
        GLES31.glBindBuffer(GLES31.GL_PIXEL_UNPACK_BUFFER, GLES31.GL_NONE);
        //TODO==========================EndUV

        //3.6-设置渲染的坐标
        GLES31.glEnableVertexAttribArray(vPosition);
        GLES31.glVertexAttribPointer(vPosition, 2, GLES31.GL_FLOAT, false, 8, vertexBuffer);
        GLES31.glEnableVertexAttribArray(vTexCoord);
        GLES31.glVertexAttribPointer(vTexCoord, 2, GLES31.GL_FLOAT, false, 8, textureBuffer);

        //3.7-绘制
        GLES31.glDrawArrays(GLES31.GL_TRIANGLE_STRIP, 0, 4);
        //3.8-禁用顶点属性数组
        frame = null;
        GLES31.glDisableVertexAttribArray(vPosition);
        GLES31.glDisableVertexAttribArray(vTexCoord);
        //3.9-解绑
        GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, GLES31.GL_NONE);
        GLES31.glBindBuffer(GLES31.GL_ARRAY_BUFFER, GLES31.GL_NONE);
        //Log.d(TAG, "renderTime=" + (System.currentTimeMillis() - start));
    }

}
