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

#include <assert.h>
#include <jni.h>
#include <pthread.h>

#include <android/native_window_jni.h>

#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>

#include <media/NdkImageReader.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaMuxer.h>

#include "messages-internal.h"

static ANativeWindow *theNativeWindow;
static ACameraDevice *cameraDevice;
static ACaptureRequest *captureRequest;
static ACameraOutputTarget *cameraOutputTarget;
static ACaptureSessionOutput *sessionOutput;
static ACaptureSessionOutputContainer *captureSessionOutputContainer;
static ACameraCaptureSession *captureSession;
static AImageReader *imageReader;
static ANativeWindow *imageReaderWindow;
static ACameraOutputTarget *imageReaderOutputTarget;
static ACaptureSessionOutput *imageReaderSessionOutput;
static AImageReader_ImageListener imageReaderListener;
static AMediaCodec *mediaCodec;
static AMediaMuxer *mediaMuxer;
static ssize_t tidx = -1;
static pthread_mutex_t lock;

static ACameraDevice_StateCallbacks deviceStateCallbacks;
static ACameraCaptureSession_stateCallbacks captureSessionStateCallbacks;

static void camera_device_on_disconnected(void *context, ACameraDevice *device) {
    LOGI("Camera(id: %s) is diconnected.\n", ACameraDevice_getId(device));
}

static void camera_device_on_error(void *context, ACameraDevice *device, int error) {
    LOGE("Error(code: %d) on Camera(id: %s).\n", error, ACameraDevice_getId(device));
}

static void capture_session_on_ready(void *context, ACameraCaptureSession *session) {
    LOGI("Session is ready.\n");
}

static void capture_session_on_active(void *context, ACameraCaptureSession *session) {
    LOGI("Session is activated.\n");
}


extern "C" {
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_stopPreview(JNIEnv *env,
                                                                                    jclass clazz);
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_startPreview(JNIEnv *env,
                                                                                     jclass clazz,
                                                                                     jobject surface);
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_stopRecording(JNIEnv *env,
                                                                                      jclass clazz);
JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_startRecording(JNIEnv *env,
                                                                                       jclass clazz,
                                                                                       int fd,
                                                                                       jobject surface);
}

static void openCamera(ACameraDevice_request_template templateId)
{
    ACameraIdList *cameraIdList = NULL;
    ACameraMetadata *cameraMetadata = NULL;

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

    LOGI("Trying to open Camera2 (id: %s, num of camera : %d)\n", selectedCameraId,
         cameraIdList->numCameras);

    camera_status = ACameraManager_getCameraCharacteristics(cameraManager, selectedCameraId,
                                                            &cameraMetadata);

    if (camera_status != ACAMERA_OK) {
        LOGE("Failed to get camera meta data of ID:%s\n", selectedCameraId);
    }

    deviceStateCallbacks.onDisconnected = camera_device_on_disconnected;
    deviceStateCallbacks.onError = camera_device_on_error;

    camera_status = ACameraManager_openCamera(cameraManager, selectedCameraId,
                                              &deviceStateCallbacks, &cameraDevice);

    if (camera_status != ACAMERA_OK) {
        LOGE("Failed to open camera device (id: %s)\n", selectedCameraId);
    }

    camera_status = ACameraDevice_createCaptureRequest(cameraDevice, templateId,
                                                       &captureRequest);

    if (camera_status != ACAMERA_OK) {
        LOGE("Failed to create preview capture request (id: %s)\n", selectedCameraId);
    }

    ACaptureSessionOutputContainer_create(&captureSessionOutputContainer);

    captureSessionStateCallbacks.onReady = capture_session_on_ready;
    captureSessionStateCallbacks.onActive = capture_session_on_active;

    ACameraMetadata_free(cameraMetadata);
    ACameraManager_deleteCameraIdList(cameraIdList);
    ACameraManager_delete(cameraManager);
}

static void closeCamera(void)
{
    camera_status_t camera_status = ACAMERA_OK;

    if (captureRequest != NULL) {
        ACaptureRequest_free(captureRequest);
        captureRequest = NULL;
    }

    if (cameraOutputTarget != NULL) {
        ACameraOutputTarget_free(cameraOutputTarget);
        cameraOutputTarget = NULL;
    }

    if (cameraDevice != NULL) {
        camera_status = ACameraDevice_close(cameraDevice);

        if (camera_status != ACAMERA_OK) {
            LOGE("Failed to close CameraDevice.\n");
        }
        cameraDevice = NULL;
    }

    if (sessionOutput != NULL) {
        ACaptureSessionOutput_free(sessionOutput);
        sessionOutput = NULL;
    }

    if (captureSessionOutputContainer != NULL) {
        ACaptureSessionOutputContainer_free(captureSessionOutputContainer);
        captureSessionOutputContainer = NULL;
    }

    LOGI("Close Camera\n");
}

JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_startPreview(JNIEnv *env,
                                                                                     jclass clazz,
                                                                                     jobject surface) {
    theNativeWindow = ANativeWindow_fromSurface(env, surface);

    openCamera(TEMPLATE_PREVIEW);

    LOGI("Surface is prepared in %p.\n", surface);

    ACameraOutputTarget_create(theNativeWindow, &cameraOutputTarget);
    ACaptureRequest_addTarget(captureRequest, cameraOutputTarget);

    ACaptureSessionOutput_create(theNativeWindow, &sessionOutput);
    ACaptureSessionOutputContainer_add(captureSessionOutputContainer, sessionOutput);

    ACameraDevice_createCaptureSession(cameraDevice, captureSessionOutputContainer,
                                       &captureSessionStateCallbacks, &captureSession);

    ACameraCaptureSession_setRepeatingRequest(captureSession, NULL, 1, &captureRequest, NULL);
}

JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_stopPreview(JNIEnv *env,
                                                                                    jclass clazz) {
    closeCamera();
    if (theNativeWindow != NULL) {
        ANativeWindow_release(theNativeWindow);
        theNativeWindow = NULL;
    }
}

JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_stopRecording(JNIEnv *env,
                                                                                      jclass clazz)
{
    if (mediaCodec != NULL) {
        AMediaCodec_stop(mediaCodec);
        AMediaCodec_delete(mediaCodec);
        mediaCodec = NULL;
    }

    if (mediaMuxer != NULL) {
       AMediaMuxer_stop(mediaMuxer);
       AMediaMuxer_delete(mediaMuxer);
    }

    if (imageReaderSessionOutput != NULL) {
        ACaptureSessionOutput_free(imageReaderSessionOutput);
        imageReaderSessionOutput = NULL;
    }

    if (imageReaderOutputTarget != NULL) {
        ACameraOutputTarget_free(imageReaderOutputTarget);
        imageReaderOutputTarget = NULL;
    }

    AImageReader_delete(imageReader);

    closeCamera();

    if (theNativeWindow != NULL) {
        ANativeWindow_release(theNativeWindow);
        theNativeWindow = NULL;
    }

    pthread_mutex_destroy(&lock);
}


static void image_reader_on_image_available(void *context,
                                            AImageReader *reader)
{
    media_status_t media_status;
    AImage *image = NULL;

    media_status = AImageReader_acquireLatestImage(reader, &image);
    if (media_status != AMEDIA_OK) {
        LOGI("No image available.");
        return;
    }

   size_t bufferSize = 0;
   int inputBufferIdx = AMediaCodec_dequeueInputBuffer(mediaCodec, -1);
   uint8_t *inputBuffer = AMediaCodec_getInputBuffer(mediaCodec, inputBufferIdx, &bufferSize);

   if (inputBufferIdx >= 0) {
   LOGI("dequeue input buffer (size: %u)", bufferSize);
   int dataSize = 0;
   AImage_getPlaneData(image, 0, &inputBuffer, &dataSize);
   LOGI("copying image buffer (size: %d)", dataSize);

   int64_t timestamp = 0;
   AImage_getTimestamp(image, &timestamp);

   AMediaCodec_queueInputBuffer(mediaCodec, inputBufferIdx, 0, bufferSize, timestamp, 0);
   }

   AMediaCodecBufferInfo bufferInfo;
   int idx = AMediaCodec_dequeueOutputBuffer(mediaCodec, &bufferInfo, -1);

   LOGI("dequeue output buffer idx: %d", idx);

   if (idx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
       AMediaFormat *format = AMediaCodec_getOutputFormat(mediaCodec);

       tidx = AMediaMuxer_addTrack(mediaMuxer, format);
       LOGI("added track tidx: %d (format: %s)", tidx, AMediaFormat_toString(format));

       AMediaMuxer_start(mediaMuxer);
       AMediaFormat_delete(format);

   } else if (idx >= 0) {

     uint8_t *outputBuffer = AMediaCodec_getOutputBuffer(mediaCodec, idx, &bufferSize);

     if (tidx >= 0 && bufferInfo.size > 0) {
       pthread_mutex_lock(&lock);

       LOGI("sending buffer to tidx: %d ptr: %p (info->offset: %d, info->size: %d)", tidx, outputBuffer, bufferInfo.offset, bufferInfo.size);
       AMediaMuxer_writeSampleData(mediaMuxer, tidx, outputBuffer, &bufferInfo);

       pthread_mutex_unlock(&lock);
     }

     AMediaCodec_releaseOutputBuffer(mediaCodec, idx, false);
   }

   AImage_delete(image);
}

JNIEXPORT void JNICALL Java_org_freedesktop_nativecamera2_NativeCamera2_startRecording(JNIEnv *env,
                                                                                       jclass clazz,
                                                                                       int fd,
                                                                                       jobject surface)
{
    int32_t width, height;
    media_status_t media_status;

    pthread_mutex_init(&lock, NULL);

    theNativeWindow = ANativeWindow_fromSurface(env, surface);
    mediaMuxer = AMediaMuxer_new(fd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);

    width = 640; //ANativeWindow_getWidth(theNativeWindow);
    height = 480; //ANativeWindow_getHeight(theNativeWindow);

    LOGI("Setting ImageReader (w: %d, h: %d)\n", width, height);

    AImageReader_new(width, height,
                     AIMAGE_FORMAT_YUV_420_888,
                     5, /* maxImages */
                     &imageReader);

    media_status = AImageReader_getWindow(imageReader, &imageReaderWindow);
    if (media_status != AMEDIA_OK) {
       LOGE("Failed to get a native window from image reader.");
       return;
    }

    imageReaderListener.onImageAvailable = image_reader_on_image_available;
    AImageReader_setImageListener(imageReader, &imageReaderListener);

    mediaCodec = AMediaCodec_createEncoderByType("video/avc");

    AMediaFormat *mediaFormat = AMediaFormat_new();

    /** These are mandatory to configure mediacodec. */
    AMediaFormat_setString(mediaFormat, AMEDIAFORMAT_KEY_MIME, "video/avc");
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_BIT_RATE, 125000);
    AMediaFormat_setFloat(mediaFormat, AMEDIAFORMAT_KEY_FRAME_RATE, 15.0);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 5);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, 0x7f420888 /* COLOR_FormatYUV420Flexible */);

    media_status = AMediaCodec_configure(mediaCodec, mediaFormat, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE); 
    if (media_status != AMEDIA_OK) {
        LOGE("Failed to configure media codec(return code: %x)", media_status);
    }


    AMediaFormat_delete(mediaFormat);

    openCamera(TEMPLATE_STILL_CAPTURE);

    ACameraOutputTarget_create(theNativeWindow, &cameraOutputTarget);
    ACaptureRequest_addTarget(captureRequest, cameraOutputTarget);

    ACameraOutputTarget_create(imageReaderWindow, &imageReaderOutputTarget);
    ACaptureRequest_addTarget(captureRequest, imageReaderOutputTarget);

    ACaptureSessionOutput_create(theNativeWindow, &sessionOutput);
    ACaptureSessionOutputContainer_add(captureSessionOutputContainer, sessionOutput);

    ACaptureSessionOutput_create(imageReaderWindow, &imageReaderSessionOutput);
    ACaptureSessionOutputContainer_add(captureSessionOutputContainer, imageReaderSessionOutput);

    ACameraDevice_createCaptureSession(cameraDevice, captureSessionOutputContainer,
                                       &captureSessionStateCallbacks, &captureSession);

    ACameraCaptureSession_setRepeatingRequest(captureSession, NULL, 1, &captureRequest, NULL);

    AMediaCodec_start(mediaCodec);
}
