/*
 * Copyright (C) 2016, Collabora Ltd.
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

#include <assert.h>
#include <jni.h>
#include <pthread.h>

#include <android/log.h>
#include <android/native_window_jni.h>
#include <camera/NdkCameraManager.h>

static ANativeWindow *theNativeWindow;

extern "C" {
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_shutdown(JNIEnv *env,
                                                                                 jclass clazz);
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_setSurface(JNIEnv *env,
                                                                                   jclass clazz,
                                                                                   jobject surface);
}

JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_shutdown(JNIEnv *env,
                                                                                 jclass clazz) {
    if (theNativeWindow != NULL) {
        ANativeWindow_release(theNativeWindow);
        theNativeWindow = NULL;
    }
}

JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_setSurface(JNIEnv *env,
                                                                                   jclass clazz,
                                                                                   jobject surface) {
    theNativeWindow = ANativeWindow_fromSurface(env, surface);
}
