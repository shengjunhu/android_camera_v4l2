package com.hsj.sample;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.hsj.camera.CameraAPI;
import com.hsj.camera.CameraView;
import com.hsj.camera.IFrameCallback;
import com.hsj.camera.IRender;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:MainActivity
 * @Desc:
 */
public final class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    //RGB: Usb camera productId and vendorId
    private static final int RGB_PID = 12384;
    private static final int RGB_VID = 3034;
    //RGB: frame of width and height
    private static final int RGB_WIDTH = 1280;
    private static final int RGB_HEIGHT = 800;
    //IR: Usb camera productId and vendorId
    private static final int IR_PID = 25446;
    private static final int IR_VID = 3141;
    //IR: frame of width and height
    private static final int IR_WIDTH = 640;
    private static final int IR_HEIGHT = 400;
    //IRender
    private IRender renderRGB, renderIR;
    //CameraAPI
    private CameraAPI cameraRGB, cameraIR;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //RGB
        CameraView cameraView = findViewById(R.id.preview);
        renderRGB = cameraView.getRender(RGB_WIDTH, RGB_HEIGHT, CameraView.NV12);
        //IR
        //CameraView cameraView2 = findViewById(R.id.preview2);
        //renderIR = cameraView2.getRender(IR_WIDTH, IR_HEIGHT, CameraView.YUYV);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (renderRGB != null) {
            renderRGB.onRender(true);
        }
        if (renderIR != null) {
            renderIR.onRender(true);
        }
    }

    @Override
    protected void onPause() {
        if (renderRGB != null) {
            renderRGB.onRender(false);
        }
        if (renderIR != null) {
            renderIR.onRender(false);
        }
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        if (renderRGB!=null){
            renderRGB.release();
        }
        if (renderIR!=null){
            renderIR.release();
        }
    }

//==================================================================================================

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
            this.cameraRGB.setAutoExposure(true);
            this.cameraRGB.start();

            this.cameraIR.setFrameSize(IR_WIDTH, IR_HEIGHT, CameraAPI.PIXEL_FORMAT_YUYV);
            //this.cameraIR.setFrameCallback(irCallback);
            //this.cameraIR.setAutoExposure(false);
            //625->5ms, 312->4ms, 156->3.0ms, 78->2.5ms, 39->1.8ms, 20->1.4ms, 10->1.0ms, 5->0.6ms, 2->0.20ms, 1->0.06ms
            //this.cameraIR.setExposureLevel(156);
            this.cameraIR.start();
        }
    }

    public void stop(View view) {
        if (this.cameraRGB != null) {
            this.cameraRGB.stop();
            this.cameraIR.stop();
        }
    }

    public void destroy(View view) {
        if (this.cameraRGB != null) {
            this.cameraRGB.destroy();
            this.cameraIR.destroy();
            this.cameraRGB = null;
            this.cameraIR = null;
        }
    }

    private void showToast(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

//==================================================================================================

    private final IFrameCallback rgbCallback = frame -> renderRGB.updatePreview(frame);

    private final IFrameCallback irCallback = frame -> renderIR.updatePreview(frame);

//==================================================================================================

}