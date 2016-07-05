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

#include <android/native_window_jni.h>
#include <camera/NdkCameraManager.h>

#include "messages-internal.h"

static ANativeWindow *theNativeWindow;
static ACameraDevice_StateCallbacks deviceStateCallbacks;

static void camera_device_on_disconnected (void *context, ACameraDevice *device) {
    LOGI("Camera(id: %s) is diconnected.\n", ACameraDevice_getId(device));
}

static void camera_device_on_error (void *context, ACameraDevice *device, int error) {
    LOGE("Error(code: %d) on Camera(id: %s).\n", error, ACameraDevice_getId(device));
}


extern "C" {
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_openCamera(JNIEnv *env,
                                                                                   jclass clazz);
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_closeCamera(JNIEnv *env,
                                                                                   jclass clazz);
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_shutdown(JNIEnv *env,
                                                                                 jclass clazz);
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_setSurface(JNIEnv *env,
                                                                                   jclass clazz,
                                                                                   jobject surface);
}

JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_openCamera(JNIEnv *env,
                                                                                   jclass clazz) {
    ACameraIdList *cameraIdList = NULL;
    ACameraMetadata *cameraMetadata = NULL;
    ACameraDevice *cameraDevice = NULL;
    const char *selectedCameraId = NULL;
    camera_status_t camera_status = ACAMERA_OK;
    ACameraManager *cameraManager = ACameraManager_create();

    camera_status = ACameraManager_getCameraIdList(cameraManager, &cameraIdList);
    if (camera_status != ACAMERA_OK) {
        LOGE("Failed to get camera id list (reason: %d)\n", camera_status);
        return;
    }

    if (cameraIdList->numCameras < 1) {
        LOGE("No camera device detected.\n");
        return;
    }

    selectedCameraId = cameraIdList->cameraIds[0];

    LOGI("Trying to open Camera2 (id: %s, num of camera : %d)\n", selectedCameraId, cameraIdList->numCameras);

    camera_status = ACameraManager_getCameraCharacteristics(cameraManager, selectedCameraId, &cameraMetadata);

    if (camera_status != ACAMERA_OK) {
        LOGE("Failed to get camera meta data of ID:%s\n", selectedCameraId);
    }

    deviceStateCallbacks.onDisconnected = camera_device_on_disconnected;
    deviceStateCallbacks.onError = camera_device_on_error;

    camera_status = ACameraManager_openCamera(cameraManager, selectedCameraId, &deviceStateCallbacks, &cameraDevice);

    if (camera_status != ACAMERA_OK) {
        LOGE("Failed to open camera device (id: %s)\n", selectedCameraId);
    }

    ACameraDevice_close(cameraDevice);
    ACameraMetadata_free(cameraMetadata);
    ACameraManager_deleteCameraIdList(cameraIdList);
    ACameraManager_delete(cameraManager);
}

JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_closeCamera(JNIEnv *env,
                                                                                    jclass clazz) {
    LOGI("Close Camera2\n");
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
    LOGI("Surface is prepared in %p.\n", surface);
}
