package org.freedesktop.nativecamera2;

import android.app.Activity;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class NativeCamera2 extends Activity {

  public static native void setSurface(Surface surface);

  static {
    System.loadLibrary ("native-camera2-jni");
  } 
}
