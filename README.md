Android Webcam Library
======================

This project is an Android library to provide an API to access an external USB
webcam that supports UVC. The library provides an in-process service
(`WebcamManager`) so that multiple parts of an application can share the same
webcam.

The library also provides an example View that can be used directly in a layout,
the `WebcamPreview`.

## Dependencies

* [Android SDK](http://developer.android.com/sdk/index.html)
* [Android NDK](http://developer.android.com/tools/sdk/ndk/index.html)
* [Android Webcam library](https://github.com/openxc/android-webcam)
* USB Webcam supporting UVC and a 640x480 resolution in the YUYV format.

## Installation

* Clone the [Git repository](https://github.com/openxc/android-webcam)
* Run `ndk-build` in the `android-webcam/android-webcam-library` folder to
  compile the native library
* Reference the library from your Android application

**With Maven:**

To install the library to your local Maven repository, so other apps can
reference it:

    $ cd android-webcam
    $ cd android-webcam-library; ndk-build && cd ..
    $ mvn install -pl android-webcam-library -am

## Usage

To install the example application, first install the library, then:

    $ mvn package android:deploy android:run -pl example

To just display the webcam image, add this to your app's layout:

    <com.ford.openxc.webcam.WebcamPreview
            android:id="@+id/cp" android:layout_width="fill_parent"
            android:layout_height="0dip" android:layout_weight="1"/>

If you want to modify the image at all, subclass `WebcamPreview` and override
the `run()` method to make any modifications to the iamge before rendering it to
the canvas. The `WebcamManager` service has a `getImage()` method which returns
a Bitmap of the latest frame from the camera.

## USB Webcam Support

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

## Contributors

This project was originally based on a JNI implementation for UVC webcams by
[neuralassembly](https://bitbucket.org/neuralassembly/simplewebcam).

See the CONTRIBUTORS file for other contributors.

## License

Copyright (c) 2011-2013 Ford Motor Company
Licensed under the BSD license.
