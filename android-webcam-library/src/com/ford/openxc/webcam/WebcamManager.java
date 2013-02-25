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
    private Webcam mWebcam;

    public class WebcamBinder extends Binder {
        public WebcamManager getService() {
            return WebcamManager.this;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "Service starting");

        mWebcam = new NativeWebcam("/dev/video0");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "Service being destroyed");
        mWebcam.stop();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "Service binding in response to " + intent);
        return mBinder;
    }

    public Bitmap getFrame() {
        if(!mWebcam.isAttached()) {
            stopSelf();
        }
        return mWebcam.getFrame();
    }
}
