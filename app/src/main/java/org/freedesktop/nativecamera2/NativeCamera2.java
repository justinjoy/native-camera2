/*
 * Copyright (C) 2016-2017, Collabora Ltd.
 *   Author: Justin Kim <justin.kim@collabora.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.freedesktop.nativecamera2;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;

public class NativeCamera2 extends Activity {

    static final String TAG = "NativeCamera2";

    private static final int PERMISSION_REQUEST_CAMERA = 1;

    public static native void startPreview(Surface surface);

    public static native void stopPreview();

    public static native void startExtraView(Surface surface);

    public static native void stopExtraView();

    LayoutInflater extraViewLayoutInflater = null;

    boolean isBurstModeOn = false;

    static {
        System.loadLibrary("native-camera2-jni");
    }

    SurfaceView surfaceView;
    SurfaceHolder surfaceHolder;

    SurfaceView extraView;
    SurfaceHolder extraViewHolder;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.main);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {

            ActivityCompat.requestPermissions(
                    this,
                    new String[] { Manifest.permission.CAMERA },
                    PERMISSION_REQUEST_CAMERA);
            return;
        }

        surfaceView = (SurfaceView) findViewById(R.id.surfaceview);
        surfaceHolder = surfaceView.getHolder();

        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {

                Log.v(TAG, "surface created.");
                startPreview(holder.getSurface());
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                stopPreview();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.v(TAG, "format=" + format + " w/h : (" + width + ", " + height + ")");
            }
        });


        extraViewLayoutInflater = LayoutInflater.from(getBaseContext());

        View view = extraViewLayoutInflater.inflate(R.layout.extraviewlayout, null);
        ViewGroup.LayoutParams layoutParamsControl
                = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        this.addContentView(view, layoutParamsControl);

        extraView = (SurfaceView) findViewById(R.id.extraview);
        extraView.setVisibility(View.INVISIBLE);

        extraViewHolder = extraView.getHolder();
        extraViewHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                startExtraView(extraViewHolder.getSurface());

            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                stopExtraView();
            }
        });

        surfaceView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                isBurstModeOn = !isBurstModeOn;

                if (isBurstModeOn) {
                    extraView.setVisibility(View.VISIBLE);
                } else {
                    extraView.setVisibility(View.INVISIBLE);
                }
            }
        });
    }

    @Override
    protected void onDestroy() {
        stopPreview();
        super.onDestroy();
    }


}
