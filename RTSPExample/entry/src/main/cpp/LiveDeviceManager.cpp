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

#include "LiveLog.h"
#include "LiveDeviceManager.h"

static inline void _CameraManagerStatusCallback(
    Camera_Manager* cameraManager, Camera_StatusInfo* status) {
    LLogI(
        "[%{public}s] status: %{public}d",
        status->camera->cameraId,
        status->status);
}

static void _LiveCameraManagerDestroy(LiveCameraManager* liveCM) {
    LLogI("%{public}s enter.", __FUNCTION__);
    if (liveCM->camDevices && liveCM->camDeviceCnt > 0) {
        OH_CameraManager_DeleteSupportedCameras(
            liveCM->camMgr, liveCM->camDevices, liveCM->camDeviceCnt);
    }
    if (liveCM->camMgr) {
        OH_Camera_DeleteCameraManager(liveCM->camMgr);
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}

static bool _LiveCameraManagerCreate(LiveCameraManager* liveCM) {
    LLogI("%{public}s enter.", __FUNCTION__);

    Camera_ErrorCode ret = CAMERA_OK;
    liveCM->camMgrCb.onCameraStatus = _CameraManagerStatusCallback;

    ret = OH_Camera_GetCameraManager(&liveCM->camMgr);
    if (!liveCM->camMgr || ret != CAMERA_OK) {
        LLogE("OH_Camera_GetCameraManager failed. Reason: (%{public}%d)", ret);
        _LiveCameraManagerDestroy(liveCM);
        return false;
    }
    ret = OH_CameraManager_GetSupportedCameras(
        liveCM->camMgr, &liveCM->camDevices, &liveCM->camDeviceCnt);
    if (!liveCM->camDevices || liveCM->camDeviceCnt < 0 || ret != CAMERA_OK) {
        LLogE(
            "OH_CameraManager_GetSupportedCameras failed. Reason: "
            "(%{public}%d)",
            ret);
        _LiveCameraManagerDestroy(liveCM);
        return false;
    }
    LLogI("Dump camera info:");
    for (int idx = 0; idx < liveCM->camDeviceCnt; idx++) {
        if (!liveCM->camDevices[idx].cameraId) {
            continue;
        }
        LLogI("cameraId = %{public}s ", liveCM->camDevices[idx].cameraId);
        LLogI("cameraType = %{public}d ", liveCM->camDevices[idx].cameraType);
        LLogI(
            "cameraPosition = %{public}d ",
            liveCM->camDevices[idx].cameraPosition);
        LLogI(
            "connectionType = %{public}d ",
            liveCM->camDevices[idx].connectionType);
    }
    ret = OH_CameraManager_RegisterCallback(liveCM->camMgr, &liveCM->camMgrCb);
    if (ret != CAMERA_OK) {
        LLogE(
            "OH_CameraManager_RegisterCallback failed. Reason: (%{public}%d)",
            ret);
        _LiveCameraManagerDestroy(liveCM);
        return false;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
    return true;
}

static void
_LiveDevicesInfoUpdate(LiveDeviceManager* liveDM, LiveDeviceType devType) {
    if (devType & LIVE_CAMERA_TYPE) {
        for (int devNo = 0; devNo < liveDM->liveCM.camDeviceCnt; devNo++) {
            if (!liveDM->liveCM.camDevices[devNo].cameraId) {
                continue;
            }
            liveDM->liveDIs.emplace_back(LiveDeviceInfo{
                devNo, liveDM->liveCM.camDevices[devNo].cameraId, devType});
        }
    }
    return;
}

LiveDeviceManager* LiveDeviceManagerCreate(LiveDeviceType devType) {
    LLogI("%{public}s enter.", __FUNCTION__);

    LiveDeviceManager* liveDM = new LiveDeviceManager;
    if (!liveDM) {
        return nullptr;
    }
    LLogI(
        "The device type is: %{public}s",
        devType == LIVE_CAMERA_TYPE ? "Camera" : "Unknown");

    if (devType & LIVE_CAMERA_TYPE) {
        if (!_LiveCameraManagerCreate(&liveDM->liveCM)) {
            LiveDeviceManagerDestroy(liveDM);
            return nullptr;
        }
        _LiveDevicesInfoUpdate(liveDM, devType);
    }
    LLogI("%{public}s leave.", __FUNCTION__);
    return liveDM;
}

std::vector<LiveDeviceInfo>
LiveDeviceManagerDevicesInfoRetrieve(LiveDeviceManager* liveDM) {
    return liveDM->liveDIs;
}

void LiveDeviceManagerDestroy(LiveDeviceManager* liveDM) {
    LLogI("%{public}s enter.", __FUNCTION__);
    if (liveDM) {
        _LiveCameraManagerDestroy(&liveDM->liveCM);
        delete liveDM;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}