package com.ford.openxc.webcam;

import android.app.Service;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class WebcamManager extends Service {

    private static String TAG = "WebcamManager";

    private IBinder mBinder = new WebcamBinder();

    private int cameraId = 0;

    // This definition also exists in ImageProc.h.
    // Webcam must support the resolution 640x480 with YUYV format.
    static final int IMG_WIDTH = 640;
    static final int IMG_HEIGHT = 480;

    // TODO do these have to be public?
    private native int prepareCamera(int videoid);
    private native void processCamera();
    private native void stopCamera();
    private native void pixeltobmp(Bitmap bitmap);

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

        // /dev/videox (x=cameraId)
        prepareCamera(cameraId);
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

    public Bitmap getImage() {

        processCamera();
        // TODO where do we get the height and width, and is it wasteful to keep
        // creating the bitmap over and over again?
        // TODO maybe it's getImage(Bitmap) so you can decide what size you
        // want. do we expect it to be of certain dimensions in JNI code?
        Bitmap bitmap = Bitmap.createBitmap(IMG_WIDTH, IMG_HEIGHT,
                Bitmap.Config.ARGB_8888);
        if(cameraAttached()) {
            pixeltobmp(bitmap);
        }
        return bitmap;
    }
}
