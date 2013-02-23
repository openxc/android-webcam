package com.ford.openxc.webcam;

import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;

import android.app.Service;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class WebcamManager extends Service {

    private static String TAG = "WebcamManager";
    private final int CAMERA_ID = 0;

    private IBinder mBinder = new WebcamBinder();
    private Bitmap mBitmap;

    // This definition also exists in ImageProc.h.
    // Webcam must support the resolution 640x480 with YUYV format.
    static final int IMG_WIDTH = 640;
    static final int IMG_HEIGHT = 480;

    private native int prepareCamera(String deviceName, int width, int height);
    private native void processCamera();
    private native void stopCamera();
    private native void pixeltobmp(Bitmap bitmap);
    private native void setFramesize(int width, int height);

    public native boolean cameraAttached();

    static {
        System.loadLibrary("webcam");
    }

    public class WebcamBinder extends Binder {
        public WebcamManager getService() {
            return WebcamManager.this;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "Service starting");

        boolean deviceReady = true;

        String deviceName = "/dev/video" + CAMERA_ID;
        File deviceFile = new File(deviceName);
        if(deviceFile.exists()) {
            if(!deviceFile.canExecute()) {
                Log.d(TAG, "Insufficient permissions on " + deviceName +
                        " -- will try and change as root");
                try {
                    Process p = Runtime.getRuntime().exec("su");
                    DataOutputStream stream = new DataOutputStream(p.getOutputStream());
                    stream.writeBytes("chmod 0666 " + deviceName + "\n");
                    stream.flush();
                    stream.writeBytes("exit\n");
                    stream.flush();
                    p.waitFor();

                    if(p.exitValue() != 0) {
                        Log.w(TAG, "Unable to fix permissions on " + deviceName);
                        deviceReady = false;
                    }
                } catch(IOException e) {
                    Log.w(TAG, "Unable to fix permissions - " +
                            "device may not be rooted", e);
                    deviceReady = false;
                } catch(InterruptedException e) {
                    Log.w(TAG, "Unable to fix permissions", e);
                    deviceReady = false;
                }
            }
        } else {
            Log.w(TAG, deviceName + " does not exist");
            deviceReady = false;
        }

        if(deviceReady) {
            prepareCamera(deviceName, 640, 480);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "Service being destroyed");
        stopCamera();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "Service binding in response to " + intent);
        return mBinder;
    }

    public void setCameraFramesize(int width, int height) {
        Log.i(TAG, "Setting frame size to " + width + "x" + height);
        setFramesize(width, height);
        mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    }

    public Bitmap getImage() {
        processCamera();
        // TODO maybe it's getImage(Bitmap) so you can decide what size you
        // want. do we expect it to be of certain dimensions in JNI code?
        if(cameraAttached()) {
            pixeltobmp(mBitmap);
        }
        return mBitmap;
    }
}
