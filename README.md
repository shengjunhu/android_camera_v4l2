# android_camera_v4l2
V4L2 camera for android

### 简介
- 1、基于V4L2协议封装Android相机
- 2、支持设置分辨率、图像获取的原始格式：MJPEG、YUYV
- 3、采用NdkMediaCodec做MJPEG解码，如果设备不支持硬解，推荐libjpeg-turbo,替换HwDecoder即可
- 4、OpenGL渲染图像，支持YUYV、NV12、NV21、DEPTH等数据
