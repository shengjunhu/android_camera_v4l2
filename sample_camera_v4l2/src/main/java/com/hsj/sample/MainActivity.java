package com.hsj.sample;

import android.content.Context;
import android.graphics.Color;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.opengl.GLES20;
import android.os.Bundle;
import android.text.TextUtils;
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
import com.hsj.camera.ISurfaceCallback;

import java.io.Closeable;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.HashMap;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:MainActivity
 * @Desc:
 */
public final class MainActivity extends AppCompatActivity implements ISurfaceCallback {

    private static final String TAG = "MainActivity";
    //RGB: Usb camera productId and vendorId
    private static final int RGB_PID = 37424;//37424、12384、0x0c45
    private static final int RGB_VID = 1443;//1443、3034、0x636b
    //RGB: frame of width and height
    private static final int RGB_WIDTH = 640;
    private static final int RGB_HEIGHT = 480;
    //IR: Usb camera productId and vendorId
    private static final int IR_PID = 768;
    private static final int IR_VID = 11707;
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
        findViewById(R.id.btn_create).setOnClickListener(v -> create());
        findViewById(R.id.btn_start).setOnClickListener(v -> start());
        findViewById(R.id.btn_stop).setOnClickListener(v -> stop());
        findViewById(R.id.btn_destroy).setOnClickListener(v -> destroy());
        CameraView cameraView = findViewById(R.id.preview);
        render = cameraView.getRender(RGB_WIDTH, RGB_HEIGHT, CameraView.BEAUTY);
        //render = cameraView.getRender(IR_WIDTH, IR_HEIGHT, CameraView.DEPTH);
        render.setSurfaceCallback(this);

        //Print usb video productId and vendorId
        UsbManager mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> devices = mUsbManager.getDeviceList();
        for (UsbDevice device : devices.values()) {
            Log.i(TAG, "pn=" + device.getProductName() + ",pid=" + device.getProductId() + ",vid=" + device.getVendorId());
        }

        //Request /dev/video* permission
        boolean result = requestPermission();
        if (!result) Log.e(TAG, "Request permission failed");
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

    private void create() {
        if (this.cameraRGB == null) {
            CameraAPI camera = new CameraAPI();
            boolean ret = camera.create(RGB_PID, RGB_VID);
            if (ret) ret = camera.setFrameSize(RGB_WIDTH, RGB_HEIGHT, CameraAPI.FRAME_FORMAT_MJPEG);
            if (ret) this.cameraRGB = camera;
        }

        if (this.cameraIR == null) {
            CameraAPI camera = new CameraAPI();
            boolean ret = camera.create(IR_PID, IR_VID);
            if (ret) camera.setFrameSize(IR_WIDTH, IR_HEIGHT, CameraAPI.FRAME_FORMAT_DEPTH);
            if (ret) this.cameraIR = camera;
        }
    }

    private void start() {
        if (this.cameraRGB != null) {
            if (surface != null) this.cameraRGB.setPreview(surface);
            this.cameraRGB.setFrameCallback(rgbCallback);
            this.cameraRGB.start();
        }

        if (this.cameraIR != null) {
            this.cameraIR.setFrameCallback(irCallback);
            //this.cameraIR.setAutoExposure(false);
            //625->5ms, 312->4ms, 156->3.0ms, 78->2.5ms, 39->1.8ms, 20->1.4ms, 10->1.0ms, 5->0.6ms, 2->0.20ms, 1->0.06ms
            //this.cameraIR.setExposureLevel(156);
            //if (surface != null) this.cameraIR.setPreview(this.surface);
            this.cameraIR.start();
        }
    }

    private void stop() {
        if (this.cameraRGB != null) this.cameraRGB.stop();

        if (this.cameraIR != null) this.cameraIR.stop();
    }

    private void destroy() {
        if (this.cameraRGB != null) {
            this.cameraRGB.destroy();
            this.cameraRGB = null;
        }

        if (this.cameraIR != null) {
            this.cameraIR.destroy();
            this.cameraIR = null;
        }
    }

    @Override
    public void onSurface(Surface surface) {
        if (surface == null) stop();
        this.surface = surface;
    }

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
                if (dos != null) {
                    dos.close();
                }
                if (process != null) {
                    process.destroy();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        Log.d(TAG, "request video rw permission: " + result);
        return result;
    }

//==================================================================================================

    private final IFrameCallback rgbCallback = frame -> {

    };

    private final IFrameCallback irCallback = frame -> {

    };

//==================================================================================================

    private void showToast(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

    /**
     * 保存ByteBuffer到文件
     *
     * @param dstFile 存在文件
     * @param data    数据内容
     * @return 结果
     */
    public static boolean saveFile(String dstFile, ByteBuffer data) {
        if (TextUtils.isEmpty(dstFile)) return false;
        boolean result = true;
        FileChannel fc = null;
        try {
            fc = new FileOutputStream(dstFile).getChannel();
            fc.write(data);
        } catch (IOException e) {
            e.printStackTrace();
            result = false;
        } finally {
            ioClose(fc);
        }
        return result;
    }

    /**
     * 关闭IO流
     *
     * @param closeable 流
     */
    private static void ioClose(Closeable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

}