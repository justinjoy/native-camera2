package org.freedesktop.nativecamera2;

import android.app.Activity;

import android.util.Log;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.os.Bundle;

public class NativeCamera2 extends Activity {

  static final String TAG = "NativeCamera2";

  public static native void setSurface(Surface surface);
  public static native void shutdown();

  static {
    System.loadLibrary ("native-camera2-jni");
  } 

  SurfaceView surfaceView;
  SurfaceHolder surfaceHolder;

  @Override
  public void onCreate(Bundle icicle)
  {
    super.onCreate(icicle);
    setContentView(R.layout.main);

    surfaceView = (SurfaceView) findViewById(R.id.surfaceview);
    surfaceHolder = surfaceView.getHolder();

    surfaceHolder.addCallback(new SurfaceHolder.Callback() {
      @Override
      public void surfaceCreated(SurfaceHolder holder)
      {
        setSurface(holder.getSurface());
      }

      @Override
      public void surfaceDestroyed(SurfaceHolder holder)
      {
      }

      @Override
      public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
      {
        Log.v(TAG, "format=" + format + " w/h : (" + width + ", " + height + ")");
      }
    });
  }

  @Override
  protected void onDestroy()
  {
    shutdown();
    super.onDestroy();
  }
}
