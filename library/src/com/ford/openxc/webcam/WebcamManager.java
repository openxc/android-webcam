package com.ford.openxc.webcam;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class WebcamManager extends Service {

    private static String TAG = "WebcamManager";

    private IBinder mBinder = new WebcamBinder();

    // /dev/videox (x=cameraId+cameraBase) is used.
    // In some omap devices, system uses /dev/video[0-3],
    // so users must use /dev/video[4-].
    // In such a case, try cameraId=0 and cameraBase=4
    private int cameraId = 0;
    private int cameraBase = 0;

    // This definition also exists in ImageProc.h.
    // Webcam must support the resolution 640x480 with YUYV format.
    static final int IMG_WIDTH = 640;
    static final int IMG_HEIGHT = 480;

    // TODO do these have to be public?
    public native int prepareCamera(int videoid);
    public native int prepareCameraWithBase(int videoid, int camerabase);
    public native void processCamera();
    public native void stopCamera();
    public native void pixeltobmp(Bitmap bitmap);

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

        // /dev/videox (x=cameraId + cameraBase) is used
        int ret = prepareCameraWithBase(cameraId, cameraBase);
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
        pixeltobmp(bitmap);
        return bitmap;
    }
}
