/** Copyright (c) 2025, Archermind Tech. Co., Ltd. <marketing@archermind.com>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#include "LiveCamera.h"
#include "LiveLog.h"
#include "LiveServer.h"

#include "ohcamera/camera.h"
#include "ohcamera/camera_input.h"
#include "ohcamera/capture_session.h"
#include "ohcamera/preview_output.h"
#include "ohcamera/camera_manager.h"

struct LiveCamera {
    Camera_Input*            camIn = nullptr;
    CameraInput_Callbacks    camInCb;
    Camera_OutputCapability* camOutCap = nullptr;
    Camera_CaptureSession*   camCaptureSession = nullptr;
    Camera_PreviewOutput*    camPreviewOut = nullptr;
    PreviewOutput_Callbacks  camPreviewOutCb;
};

static inline void _CameraInputErrorCallback(
    const Camera_Input* cameraInput, Camera_ErrorCode errorCode) {
    LLogW("CameraInput error, Reason: %{public}d", errorCode);
}

void _CameraPreviewOutErrorCallback(
    Camera_PreviewOutput* previewOutput, Camera_ErrorCode errorCode) {
    LLogW("PreviewOutput errorCode = %{public}d", errorCode);
}

void _CameraCameraPreviewOutFrameStartCallback(
    Camera_PreviewOutput* previewOutput) {
    (void)previewOutput;
    LLogW("PreviewOutput frameStart.");
}

void _CameraCameraPreviewOutFrameEndCallback(
    Camera_PreviewOutput* previewOutput, int32_t frameCount) {
    LLogW("PreviewOutput frameCount = %{public}d", frameCount);
}

void LiveCameraStreamInfoRetrieve(int devNo, LiveStreamInfo& streamInfo) {
    Camera_OutputCapability* camOutCap = nullptr;

    Camera_ErrorCode ret = OH_CameraManager_GetSupportedCameraOutputCapability(
        liveServer.liveDM->liveCM.camMgr,
        &liveServer.liveDM->liveCM.camDevices[devNo],
        &camOutCap);
    if (!camOutCap || ret != CAMERA_OK) {
        LLogE(
            "OH_CameraManager_GetSupportedCameraOutputCapability failed, "
            "Reason: %{public}d",
            ret);
    }
    if (camOutCap->previewProfilesSize < 0) {
        streamInfo = {0};
        if (camOutCap) {
            OH_CameraManager_DeleteSupportedCameraOutputCapability(
                liveServer.liveDM->liveCM.camMgr, camOutCap);
        }
        return;
    }
    LLogI("Dump camera output capability: {");
    for (int i = 0; i < camOutCap->previewProfilesSize; i++) {
        LLogI(
            "\t devNo: %{public}d, width: %{public}d, height: %{public}d, pixelfmt: %{public}d",
            devNo,
            camOutCap->previewProfiles[i]->size.width,
            camOutCap->previewProfiles[i]->size.height,
            camOutCap->previewProfiles[i]->format);
    }
    LLogI("}");

    streamInfo.width = camOutCap->previewProfiles[0]->size.width;
    streamInfo.height = camOutCap->previewProfiles[0]->size.height;
    streamInfo.pixelFmt = camOutCap->previewProfiles[0]->format;

    if (camOutCap) {
        OH_CameraManager_DeleteSupportedCameraOutputCapability(
            liveServer.liveDM->liveCM.camMgr, camOutCap);
    }
}

LiveCamera* LiveCameraCreate(int devNo, const char* surfaceId) {
    LLogI("%{public}s enter.", __FUNCTION__);

    LiveCamera* liveCamera = new LiveCamera;
    if (!liveCamera) {
        return nullptr;
    }
    Camera_ErrorCode ret = OH_CameraManager_CreateCameraInput(
        liveServer.liveDM->liveCM.camMgr,
        &liveServer.liveDM->liveCM.camDevices[devNo],
        &liveCamera->camIn);
    if (!liveCamera->camIn || ret != CAMERA_OK) {
        LLogE(
            "OH_CameraManager_CreateCameraInput failed, Reason: %{public}d",
            ret);
        goto ERROR;
    }
    liveCamera->camInCb.onError = _CameraInputErrorCallback;
    OH_CameraInput_RegisterCallback(liveCamera->camIn, &liveCamera->camInCb);
    ret = OH_CameraInput_Open(liveCamera->camIn);
    if (ret != CAMERA_OK) {
        LLogE("OH_CameraInput_Open failed, Reason: %{public}d", ret);
        goto ERROR;
    }
    ret = OH_CameraManager_GetSupportedCameraOutputCapability(
        liveServer.liveDM->liveCM.camMgr,
        &liveServer.liveDM->liveCM.camDevices[devNo],
        &liveCamera->camOutCap);
    if (!liveCamera->camOutCap || ret != CAMERA_OK) {
        LLogE(
            "OH_CameraManager_GetSupportedCameraOutputCapability failed, "
            "Reason: %{public}d",
            ret);
        goto ERROR;
    }
    if (liveCamera->camOutCap->previewProfilesSize < 0) {
        goto ERROR;
    }
    ret = OH_CameraManager_CreatePreviewOutput(
        liveServer.liveDM->liveCM.camMgr,
        liveCamera->camOutCap->previewProfiles[0],
        surfaceId,
        &liveCamera->camPreviewOut);
    if (!liveCamera->camPreviewOut || ret != CAMERA_OK) {
        LLogE(
            "OH_CameraManager_CreatePreviewOutput failed, Reason: %{public}d",
            ret);
        goto ERROR;
    }
    liveCamera->camPreviewOutCb.onError = _CameraPreviewOutErrorCallback;
    liveCamera->camPreviewOutCb.onFrameStart =
        _CameraCameraPreviewOutFrameStartCallback;
    liveCamera->camPreviewOutCb.onFrameEnd =
        _CameraCameraPreviewOutFrameEndCallback;

    ret = OH_PreviewOutput_RegisterCallback(
        liveCamera->camPreviewOut, &liveCamera->camPreviewOutCb);
    if (ret != CAMERA_OK) {
        LLogE(
            "OH_PreviewOutput_RegisterCallback failed, Reason: %{public}d",
            ret);
        goto ERROR;
    }
    ret = OH_CameraManager_CreateCaptureSession(
        liveServer.liveDM->liveCM.camMgr, &liveCamera->camCaptureSession);
    if (!liveCamera->camCaptureSession || ret != CAMERA_OK) {
        LLogE(
            "OH_CameraManager_CreateCaptureSession failed, Reason: %{public}d",
            ret);
        goto ERROR;
    }
    ret = OH_CaptureSession_BeginConfig(liveCamera->camCaptureSession);
    if (ret != CAMERA_OK) {
        LLogE("OH_CaptureSession_BeginConfig failed, Reason: %{public}d", ret);
        goto ERROR;
    }
    ret = OH_CaptureSession_AddInput(
        liveCamera->camCaptureSession, liveCamera->camIn);
    if (ret != CAMERA_OK) {
        LLogE("OH_CaptureSession_AddInput failed, Reason: %{public}d", ret);
        goto ERROR;
    }
    ret = OH_CaptureSession_AddPreviewOutput(
        liveCamera->camCaptureSession, liveCamera->camPreviewOut);
    if (ret != CAMERA_OK) {
        LLogE(
            "OH_CaptureSession_AddPreviewOutput failed, Reason: %{public}d",
            ret);
        goto ERROR;
    }
    ret = OH_CaptureSession_CommitConfig(liveCamera->camCaptureSession);
    if (ret != CAMERA_OK) {
        LLogE("OH_CaptureSession_CommitConfig failed, Reason: %{public}d", ret);
        goto ERROR;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
    return liveCamera;
ERROR:
    LiveCameraDestroy(liveCamera);
    return nullptr;
}

void LiveCameraDestroy(LiveCamera* liveCamera) {
    LLogI("%{public}s enter.", __FUNCTION__);

    if (liveCamera->camIn) {
        OH_CameraInput_Close(liveCamera->camIn);
        OH_CameraInput_Release(liveCamera->camIn);
    }
    if (liveCamera->camOutCap) {
        OH_CameraManager_DeleteSupportedCameraOutputCapability(
            liveServer.liveDM->liveCM.camMgr, liveCamera->camOutCap);
    }
    if (liveCamera->camPreviewOut) {
        OH_PreviewOutput_Stop(liveCamera->camPreviewOut);
        OH_PreviewOutput_Release(liveCamera->camPreviewOut);
    }
    if (liveCamera->camCaptureSession) {
        OH_CaptureSession_Stop(liveCamera->camCaptureSession);
        OH_CaptureSession_Release(liveCamera->camCaptureSession);
    }
    if (liveCamera) {
        delete liveCamera;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}

void LiveCameraStart(LiveCamera* liveCamera) {
    LLogI("%{public}s enter.", __FUNCTION__);

    Camera_ErrorCode ret =
        OH_CaptureSession_Start(liveCamera->camCaptureSession);
    if (ret != CAMERA_OK) {
        LLogE("OH_CaptureSession_Start failed, Reason: %{public}d", ret);
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}
