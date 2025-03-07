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
#include <cinttypes>

#include "LiveDevice.h"
#include "LiveCamera.h"
#include "LiveDeviceManager.h"
#include "LiveEncoder.h"
#include "LiveConfig.h"
#include "LiveLog.h"

#include <multimedia/player_framework/native_avformat.h>
#include <multimedia/player_framework/native_avcodec_videoencoder.h>

#define ENCODER_BITRARE_FACTOR 0.15
/**
 * According to the official documentation, if you only want a single IDR frame,
 * this value should be set to -1. However, due to an issue in our system's
 * underlying implementation, we need to work around this. Here is how we can
 * mitigate the problem.
 */
#define ENCODER_I_FRAME_INTERVAL 1000000 // 1000s

struct LiveDevice {
    int            devNo;
    uint32_t       width;
    uint32_t       height;
    char*          devName = nullptr;
    LiveDeviceType devType;
    LiveEncoder*   encInst = nullptr;
    bool           enableWatermark;

    union Capture {
        LiveCamera* camInst = nullptr;
    } capturer;
};

LiveDevice* LiveDeviceCreate(LiveDeviceInfo& devInfo) {
    LLogI("%{public}s enter.", __FUNCTION__);

    LiveDevice* liveDevice = new LiveDevice;
    if (!liveDevice) {
        return nullptr;
    }
    liveDevice->devNo = devInfo.devNo;
    liveDevice->devName = devInfo.devName;
    liveDevice->devType = devInfo.devType;
    liveDevice->encInst = LiveEncoderCreate(ENCODER_TYPE_H264);
    if (!liveDevice->encInst) {
        LiveDeviceDestroy(liveDevice);
        return nullptr;
    }
    LiveStreamInfo streamInfo;
    LiveCameraStreamInfoRetrieve(devInfo.devNo, streamInfo);
    liveDevice->width = streamInfo.width;
    liveDevice->height = streamInfo.height;
    
    LiveEncoderConfig encConfig;
    encConfig.iFrameInterval = ENCODER_I_FRAME_INTERVAL;
    encConfig.width = streamInfo.width;
    encConfig.height = streamInfo.height;
    encConfig.frameRate = LIVE_ENCODER_FRAMERARE;
    encConfig.encProfile = OH_AVCProfile::AVC_PROFILE_BASELINE;
    encConfig.rateMode = OH_VideoEncodeBitrateMode::CBR;
    encConfig.targetBitrate = encConfig.width * encConfig.height *
                              encConfig.frameRate *
                              ENCODER_BITRARE_FACTOR; // bps

    switch (streamInfo.pixelFmt) {
    case CAMERA_FORMAT_RGBA_8888:
        encConfig.pixelFmt = AV_PIXEL_FORMAT_RGBA;
        break;
    default:
        LLogW("Not supported.");
        break;
    }
    LLogI("Dump device_%{public}d's encoder configuration: {", devInfo.devNo);
    LLogI("\t width: %{public}d", encConfig.width);
    LLogI("\t height: %{public}d", encConfig.height);
    LLogI("\t iFrameInterval: %{public}d", encConfig.iFrameInterval);
    LLogI("\t frameRate: %{public}.1f", encConfig.frameRate);
    LLogI("\t rateMode: %{public}s", encConfig.rateMode == OH_VideoEncodeBitrateMode::CBR ? "CBR" : "Unknown");
    LLogI("\t targetBitrate: %{public}" PRId64 "kbps", encConfig.targetBitrate / 1000);
    LLogI("\t pixelFmt: %{public}s", encConfig.pixelFmt == AV_PIXEL_FORMAT_RGBA ? "RGBA" : "Unknown");
    LLogI("}");

    LiveEncoderConfigure(liveDevice->encInst, encConfig);

    if (devInfo.devType & LIVE_CAMERA_TYPE) {
        liveDevice->capturer.camInst = LiveCameraCreate(
            devInfo.devNo,
            LiveEncoderSurfaceIdRetrieve(liveDevice->encInst).c_str());
        if (!liveDevice->capturer.camInst) {
            LiveDeviceDestroy(liveDevice);
            return nullptr;
        }
    }
    LLogI("%{public}s leave.", __FUNCTION__);
    return liveDevice;
}

void LiveDeviceStart(LiveDevice* liveDevice) {
    LLogI("%{public}s enter.", __FUNCTION__);
    LiveEncoderStart(liveDevice->encInst);

    if (liveDevice->devType == LIVE_CAMERA_TYPE) {
        LiveCameraStart(liveDevice->capturer.camInst);
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}

void LiveDeviceDestroy(LiveDevice* liveDevice) {
    LLogI("%{public}s enter.", __FUNCTION__);
    if (liveDevice->encInst) {
        LiveEncoderDestroy(liveDevice->encInst);
    }
    if (liveDevice->devType == LIVE_CAMERA_TYPE) {
        if (liveDevice->capturer.camInst) {
            LiveCameraDestroy(liveDevice->capturer.camInst);
        }
    }
    if (liveDevice) {
        delete liveDevice;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}

LiveEncoder* LiveDeviceEncoderRetrieve(LiveDevice* liveDevice) {
    return liveDevice->encInst;
}

void LiveDeviceResolutionRetrieve(LiveDevice* liveDevice, uint32_t& width, uint32_t height){
    width = liveDevice->width;
    height = liveDevice->height;
}