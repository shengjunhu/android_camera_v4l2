package com.hsj.sample;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Toast;

import com.hsj.camera.CameraAPI;
import com.hsj.camera.CameraView;
import com.hsj.camera.RenderRGB;

import java.nio.ByteBuffer;

public final class MainActivity extends AppCompatActivity {

    private static final String TAG = "CameraAPI";
    //RGB
    private static final int RGB_PID = 12384;
    private static final int RGB_VID = 3034;
    private static final int RGB_WIDTH = 640;
    private static final int RGB_HEIGHT = 400;
    //IR
    private static final int IR_PID = 25446;
    private static final int IR_VID = 3141;
    private static final int IR_WIDTH = 640;
    private static final int IR_HEIGHT = 400;
    //CameraAPI
    private CameraAPI cameraRGB, cameraIR;
    //Surface
    private Surface surfaceRGB;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        /*SurfaceView sv_ir = findViewById(R.id.sv_ir);
        SurfaceView sv_rgb = findViewById(R.id.sv_rgb);
        sv_rgb.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                surfaceRGB = holder.getSurface();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });*/

        CameraView cameraView = findViewById(R.id.sv_rgb);
        cameraView.setRender(new RenderRGB());
    }

    private void showToast(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

    public void create(View view) {
        if (this.cameraRGB == null) {
            this.cameraRGB = new CameraAPI();
            boolean result = this.cameraRGB.create(RGB_PID, RGB_VID);
            if (!result) {
                showToast("camera open failed: rgb");
            }

            this.cameraIR = new CameraAPI();
            result = this.cameraIR.create(IR_PID, IR_VID);
            if (!result) {
                showToast("camera open failed: ir");
            }
        }
    }

    public void start(View view) {
        if (this.cameraRGB == null) {
            showToast("please open camera");
        } else {
            this.cameraRGB.setFrameSize(RGB_WIDTH, RGB_HEIGHT, CameraAPI.PIXEL_FORMAT_MJPEG);
            this.cameraRGB.setFrameCallback(rgbCallback);
            //this.cameraRGB.setPreview(surfaceRGB);
            this.cameraRGB.start();

            this.cameraIR.setFrameSize(IR_WIDTH, IR_HEIGHT, CameraAPI.PIXEL_FORMAT_YUYV);
            this.cameraIR.setFrameCallback(irCallback);
            //this.cameraIR.setAutoExposure(false);
            //625->5ms, 312->4ms, 156->3.0ms, 78->2.5ms, 39->1.8ms, 20->1.4ms, 10->1.0ms, 5->0.6ms, 2->0.20ms, 1->0.06ms
            //this.cameraIR.setExposureLevel(39);
            this.cameraIR.start();
        }
    }

    public void stop(View view) {
        if (this.cameraRGB == null) {
            showToast("please open camera");
        } else {
            this.cameraRGB.stop();
            this.cameraIR.stop();
        }
    }

    public void destroy(View view) {
        if (this.cameraRGB == null) {
            showToast("please open camera");
        } else {
            this.cameraRGB.destroy();
            this.cameraIR.destroy();
            this.cameraRGB = null;
            this.cameraIR = null;
        }
    }

    private final CameraAPI.IFrameCallback rgbCallback = frame -> Log.d(TAG, "frame rgb");

    private final CameraAPI.IFrameCallback irCallback = frame -> Log.d(TAG, "frame ir");

}