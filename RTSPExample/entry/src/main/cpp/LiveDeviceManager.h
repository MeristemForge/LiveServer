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
#pragma once

#include <vector>

#include "ohcamera/camera.h"
#include "ohcamera/camera_manager.h"

enum LiveDeviceType {
    LIVE_CAMERA_TYPE = 0x1,
};

struct LiveDeviceInfo {
    int            devNo;
    char*          devName = nullptr;
    LiveDeviceType devType;
};

struct LiveCameraManager {
    Camera_Manager*          camMgr = nullptr;
    CameraManager_Callbacks  camMgrCb;
    Camera_Device*           camDevices = nullptr;
    uint32_t                 camDeviceCnt = 0;
};

struct LiveDeviceManager {
    LiveCameraManager           liveCM;
    std::vector<LiveDeviceInfo> liveDIs;
};

extern void LiveDeviceManagerDestroy(LiveDeviceManager* liveDM);
extern LiveDeviceManager* LiveDeviceManagerCreate(LiveDeviceType devType);
extern std::vector<LiveDeviceInfo> LiveDeviceManagerDevicesInfoRetrieve(LiveDeviceManager* liveDM);
