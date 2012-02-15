package com.camera.simplewebcam;

import android.app.Activity;
import android.os.Bundle;

public class Main extends Activity {
	
	CameraPreview cp;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		cp = new CameraPreview(this);
		setContentView(cp);
	}
	
}
