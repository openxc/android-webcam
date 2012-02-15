Eclipse project for android to use USB Webcam.

To run this application, the following conditions should be satisfied.

1) The kernel is V4L2 enabled.

2) The permission of /dev/video0 is set 0666 in /ueventd.xxxx.rc

3) USB WebCam is UVC camera, and it supports 640x480 resolution with YUYV format.
