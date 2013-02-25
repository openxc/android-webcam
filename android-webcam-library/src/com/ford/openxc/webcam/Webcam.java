package com.ford.openxc.webcam;

import android.graphics.Bitmap;

public interface Webcam {
    public Bitmap getFrame();
    public void stop();
    public boolean isAttached();
}
