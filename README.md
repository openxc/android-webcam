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

If you have `/dev/video0`, then only other requirement is that the permissions
of the file are `0666`. This is the troublesome point for many Android devices -
by default the device will be inaccessible to applications.

If the file is not readable to other users, you need to root your device. Once
rooted, you can `su` and change the permissions manually with `chmod`. Each time
you attach and detach the camera, however, the device will go back to the
default restrictive permissions. If you want to permanently change the
permissions of the `/dev/video0` file, you'll need to spin a custom Android
image with the new permissions in a /ueventd.xxxx.rc file.

## Fix video0 permissions on a rooted Nexus 7

If you're running your own image, edit the file `/ueventd.rc` and re-flash:

```
/dev/video0               0666   root       root
```

If you are running stock, you need to manually change the permissions of the
video device each time you attach it:

```
su
chmod 666 /dev/video0
```

## License

Copyright (c) 2011-2013 Ford Motor Company
Licensed under the BSD license.
