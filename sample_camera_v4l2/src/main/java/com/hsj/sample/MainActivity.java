package com.hsj.sample;

import android.content.Context;
import android.graphics.Color;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.opengl.GLES20;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.hsj.camera.CameraAPI;
import com.hsj.camera.CameraView;
import com.hsj.camera.IFrameCallback;
import com.hsj.camera.IRender;

import java.io.DataOutputStream;
import java.util.HashMap;

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
    private static final int IR_PID = 25446/*768*/;
    private static final int IR_VID = 3141/*11707*/;
    //IR: frame of width and height
    private static final int IR_WIDTH = 640;
    private static final int IR_HEIGHT = 400;
    //CameraAPI
    private CameraAPI cameraRGB, cameraIR;
    //IRender
    private IRender render;
    private Surface surface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.btn_create).setOnClickListener(v->create());
        findViewById(R.id.btn_start).setOnClickListener(v->start());
        findViewById(R.id.btn_stop).setOnClickListener(v->stop());
        findViewById(R.id.btn_destroy).setOnClickListener(v->destroy());

        CameraView cameraView = findViewById(R.id.preview);
        render = cameraView.getRender(RGB_WIDTH, RGB_HEIGHT, CameraView.BEAUTY);

        /*SurfaceView preview = findViewById(R.id.preview);
        preview.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                surface = holder.getSurface();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });*/

        requestPermission();
        UsbManager mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> devices = mUsbManager.getDeviceList();
        for (UsbDevice device : devices.values()) {
            Log.i(TAG, "pid=" + device.getProductId() + ",vid=" + device.getVendorId());
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (render != null) {
            render.onRender(true);
        }
    }

    @Override
    protected void onPause() {
        if (render != null) {
            render.onRender(false);
        }
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        destroy();
    }

//==================================================================================================

    private boolean requestPermission() {
        boolean result;
        Process process = null;
        DataOutputStream dos = null;
        try {
            process = Runtime.getRuntime().exec("su");
            dos = new DataOutputStream(process.getOutputStream());
            dos.writeBytes("chmod 666 /dev/video*\n");
            dos.writeBytes("exit\n");
            dos.flush();
            result = (process.waitFor() == 0);
        } catch (Exception e) {
            e.printStackTrace();
            result = false;
        } finally {
            try {
                if (dos != null) {dos.close();}
                if (process != null) {process.destroy();}
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        Log.d(TAG, "request video rw permission: " + result);
        return result;
    }

    private void create() {
        if (this.cameraRGB == null) {
            this.cameraRGB = new CameraAPI();
            boolean result = this.cameraRGB.create(RGB_PID, RGB_VID);
            if (result) {
                this.cameraRGB.setFrameSize(RGB_WIDTH, RGB_HEIGHT, CameraAPI.FRAME_FORMAT_MJPEG);
            }else {
                showToast("camera open failed: rgb");
            }
        }

        if (this.cameraIR == null) {
            this.cameraIR = new CameraAPI();
            boolean result = this.cameraIR.create(IR_PID, IR_VID);
            if (result) {
                this.cameraIR.setFrameSize(IR_WIDTH, IR_HEIGHT, CameraAPI.FRAME_FORMAT_YUYV);
            }else {
                showToast("camera open failed: ir");
            }
        }
    }

    private void start() {
        if (this.cameraRGB == null) {
            showToast("please open camera rgb");
        } else {
            this.cameraRGB.setPreview(render.getSurface());
            this.cameraRGB.setFrameCallback(rgbCallback);
            this.cameraRGB.start();
        }

        if (this.cameraIR == null) {
            showToast("please open camera ir");
        } else {
            this.cameraIR.setFrameCallback(irCallback);
            //this.cameraIR.setAutoExposure(false);
            //625->5ms, 312->4ms, 156->3.0ms, 78->2.5ms, 39->1.8ms, 20->1.4ms, 10->1.0ms, 5->0.6ms, 2->0.20ms, 1->0.06ms
            //this.cameraIR.setExposureLevel(156);
            //this.cameraIR.setPreview(render.getSurface());
            this.cameraIR.start();
        }
    }

    private void stop() {
        if (this.cameraRGB != null) {
            this.cameraRGB.stop();
        }
        if (this.cameraIR != null) {
            this.cameraIR.stop();
        }
    }

    private void destroy() {
        if (this.cameraRGB != null) {
            this.cameraRGB.destroy();
        }
        if (this.cameraIR != null) {
            this.cameraIR.destroy();
        }
        this.cameraRGB = null;
        this.cameraIR = null;
    }

//==================================================================================================

    private final IFrameCallback rgbCallback = frame -> {};

    private final IFrameCallback irCallback = frame -> {};

//==================================================================================================

    private void showToast(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

}