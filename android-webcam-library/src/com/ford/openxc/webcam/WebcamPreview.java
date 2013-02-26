package com.ford.openxc.webcam;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.os.IBinder;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class WebcamPreview extends SurfaceView implements
        SurfaceHolder.Callback, Runnable {
    private static String TAG = "WebcamPreview";

    private Rect mViewWindow;
    private boolean mRunning = true;
    private Object mServiceSyncToken = new Object();
    private WebcamManager mWebcamManager;
    private SurfaceHolder mHolder;

    public WebcamPreview(Context context) {
        super(context);
        init();
    }

    public WebcamPreview(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        Log.d(TAG, "WebcamPreview constructed");
        setFocusable(true);

        mHolder = getHolder();
        mHolder.addCallback(this);
    }

    @Override
    public void run() {
        while(mRunning) {
            synchronized(mServiceSyncToken) {
                if(mWebcamManager == null) {
                    try {
                        mServiceSyncToken.wait();
                    } catch(InterruptedException e) {
                        break;
                    }
                }

                Bitmap bitmap = mWebcamManager.getFrame();
                Canvas canvas = mHolder.lockCanvas();
                if(canvas != null) {
                    drawOnCanvas(canvas, bitmap);
                    mHolder.unlockCanvasAndPost(canvas);
                }
            }
        }
    }

    protected void drawOnCanvas(Canvas canvas, Bitmap videoBitmap) {
        canvas.drawBitmap(videoBitmap, null, mViewWindow, null);
    }

    protected Rect getViewingWindow() {
        return mViewWindow;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "Surface created");
        mRunning = true;
        getContext().bindService(new Intent(getContext(), WebcamManager.class),
                mConnection, Context.BIND_AUTO_CREATE);
        (new Thread(this)).start();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "Surface destroyed");
        mRunning = false;

        if(mWebcamManager != null) {
            Log.i(TAG, "Unbinding from webcam manager");
            getContext().unbindService(mConnection);
            mWebcamManager = null;
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int winWidth,
            int winHeight) {
        Log.d("WebCam", "surfaceChanged");
        int width, height, dw, dh;
        if(winWidth * 3 / 4 <= winHeight) {
            dw = 0;
            dh = (winHeight - winWidth * 3 / 4) / 2;
            width = dw + winWidth - 1;
            height = dh + winWidth * 3 / 4 - 1;
        } else {
            dw = (winWidth - winHeight * 4 / 3) / 2;
            dh = 0;
            width = dw + winHeight * 4 / 3 - 1;
            height = dh + winHeight - 1;
        }
        mViewWindow = new Rect(dw, dh, width, height);
    }

    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className,
                IBinder service) {
            Log.i(TAG, "Bound to WebcamManager");
            synchronized(mServiceSyncToken) {
                mWebcamManager = ((WebcamManager.WebcamBinder)service).getService();
                mServiceSyncToken.notify();
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            Log.w(TAG, "WebcamManager disconnected unexpectedly");
            synchronized(mServiceSyncToken) {
                mRunning = false;
                mWebcamManager = null;
                mServiceSyncToken.notify();
            }
        }
    };
}
