To use a USB webcam in Android, the kernel must be compiled with `V4L2`, e.g.:

    CONFIG_VIDEO_DEV=y
    CONFIG_VIDEO_V4L2_COMMON=y
    CONFIG_VIDEO_MEDIA=y
    CONFIG_USB_VIDEO_CLASS=y
    CONFIG_V4L_USB_DRIVERS=y
    CONFIG_USB_VIDEO_CLASS_INPUT_EVDEV=y

If the file `/dev/video0` appears on the device when you plug in a video camera,
then you've got `V4L2` support. The Toshiba Thrive 10.1" and Google Nexus 7 have
been confirmed to have the module.

If you have `/dev/video0`, check the permissions in adb:

    $ adb shell
    shell@android:/ $ ls -l /dev/video0
    crw-rw---- media    camera    81,   0 2013-02-25 10:31 video0

If the file is at least `0660`, owned by the `media` user with the group
`camera`, you're good to go.

Add the `CAMERA` permissions to your app's manifest:

    <uses-permission android:name="android.permission.CAMERA" />

## License

Copyright (c) 2011-2013 Ford Motor Company
Licensed under the BSD license.
