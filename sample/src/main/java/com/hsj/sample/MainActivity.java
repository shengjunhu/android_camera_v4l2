package com.hsj.sample;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Surface;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.util.Pair;

import com.hsj.camera.CameraAPI;
import com.hsj.camera.CameraView;
import com.hsj.camera.IFrameCallback;
import com.hsj.camera.IRender;
import com.hsj.camera.ISurfaceCallback;

import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.Collection;

/**
 * @Author:Hsj
 * @Date:2021/5/10
 * @Class:MainActivity
 * @Desc:
 */
public final class MainActivity extends AppCompatActivity implements ISurfaceCallback {

    private static final String TAG = "MainActivity";
    // Usb device: productId
    private int pid;
    // Usb device: vendorId
    private int vid;
    // Dialog checked index
    private int index;
    // CameraAPI
    private CameraAPI camera;
    // IRender
    private IRender render;
    private Surface surface;
    private LinearLayout ll;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.btn_create).setOnClickListener(v -> create());
        findViewById(R.id.btn_start).setOnClickListener(v -> start());
        findViewById(R.id.btn_stop).setOnClickListener(v -> stop());
        findViewById(R.id.btn_destroy).setOnClickListener(v -> destroy());
        ll = findViewById(R.id.ll);
        CameraView cameraView = findViewById(R.id.cameraView);
        this.render = cameraView.getRender(CameraView.COMMON);
        this.render.setSurfaceCallback(this);

        //Request permission: /dev/video*
        boolean ret = requestPermission();
        showToast("Request permission: " + (ret ? "succeed" : "failed"));
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
        super.onPause();
        if (render != null) {
            render.onRender(false);
        }
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

//==========================================Menu====================================================

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        if (item.getItemId() == R.id.item_camera) {
            showSingleChoiceDialog();
        }
        return super.onOptionsItemSelected(item);
    }

//===========================================Camera=================================================

    private void create() {
        if (this.camera == null) {
            CameraAPI camera = new CameraAPI();
            boolean ret = camera.create(pid, vid);
            int[][] supportFrameSize = camera.getSupportFrameSize();
            if (supportFrameSize == null || supportFrameSize.length == 0) {
                showToast("Get support preview size failed.");
            } else {
                final int index = supportFrameSize.length / 2;
                final int width = supportFrameSize[index][0];
                final int height = supportFrameSize[index][1];
                Log.d(TAG, "width=" + width + ", height=" + height);
                if (ret) ret = camera.setFrameSize(width, height, CameraAPI.FRAME_FORMAT_MJPEG);
                if (ret) this.camera = camera;
            }
        } else {
            showToast("Camera had benn created");
        }
    }

    private void start() {
        if (this.camera != null) {
            if (surface != null) this.camera.setPreview(surface);
            this.camera.setFrameCallback(frameCallback);
            this.camera.start();
        } else {
            showToast("Camera have not create");
        }
    }

    private final IFrameCallback frameCallback = frame -> {};

    private void stop() {
        if (this.camera != null) {
            this.camera.stop();
        }
    }

    private void destroy() {
        if (this.camera != null) {
            this.camera.destroy();
            this.camera = null;
        }
    }

//=============================================Other================================================

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

    private void showSingleChoiceDialog() {
        UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        Collection<UsbDevice> values = usbManager.getDeviceList().values();
        final UsbDevice[] devices = values.toArray(new UsbDevice[]{});
        int size = devices.length;
        if (size == 0) {
            showToast("No Usb device to be found");
        } else {
            // stop and destroy
            stop();
            destroy();
            this.ll.setVisibility(View.GONE);
            // get Usb devices name
            String[] items = new String[size];
            for (int i = 0; i < size; ++i) {
                items[i] = "Device: " + devices[i].getProductName();
            }
            // dialog
            if (index >= size) index = 0;
            AlertDialog.Builder ad = new AlertDialog.Builder(this);
            ad.setTitle(R.string.select_usb_device);
            ad.setSingleChoiceItems(items, index, (dialog, which) -> index = which);
            ad.setPositiveButton(R.string.btn_confirm, (dialog, which) -> {
                this.pid = devices[index].getProductId();
                this.vid = devices[index].getVendorId();
                this.ll.setVisibility(View.VISIBLE);
            });
            ad.show();
        }
    }

    @Override
    public void onSurface(Surface surface) {
        if (surface == null) stop();
        this.surface = surface;
    }

    private void showToast(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

    private boolean saveFile(String dstFile, ByteBuffer data) {
        if (TextUtils.isEmpty(dstFile)) return false;
        boolean ret = false;
        FileChannel fc = null;
        try {
            fc = new FileOutputStream(dstFile).getChannel();
            fc.write(data);
            ret = true;
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fc != null) {
                try {
                    fc.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return ret;
    }

}