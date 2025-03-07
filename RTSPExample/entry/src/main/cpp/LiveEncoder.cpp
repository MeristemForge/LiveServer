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
#include <string>
#include <cinttypes>
#include <chrono>
#include <vector>

#include "LiveLog.h"
#include "LiveEncoder.h"
#include "LiveUtils.h"
#include "LiveConfig.h"

#include <multimedia/player_framework/native_avcodec_videoencoder.h>
#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>
#include <multimedia/player_framework/native_avformat.h>
#include <multimedia/player_framework/native_avbuffer.h>
#include <native_window/external_window.h>

struct LiveEncoder {
    OHNativeWindow*       nativeWindow = nullptr;
    LiveEncoderConfig     config;
    EncoderType           encType;
    const char*           encName = nullptr;
    OH_AVCodec*           encCore = nullptr;
    int64_t               encTargetBitrate;
    OH_AVCodecCallback    encCb;
    LiveNalUnitQueue*     encNalQueue = nullptr;
    std::vector<uint8_t>  encSPS;
    std::vector<uint8_t>  encPPS;
    std::vector<uint8_t>  encSEI;
    std::function<void()> encFrameAvailableCb;
    LiveStreamDumper*     streamDumper = nullptr;
    bool                  enableStreamDumper = LIVE_ENCODER_DUMP_ENABLE;
    bool                  enableLowLatency = false;
};

static void OnError(OH_AVCodec* codec, int32_t errorCode, void* userData) {
    (void)codec;
    (void)errorCode;
    (void)userData;
    LLogF("On encoder error, error code: %{public}d", errorCode);
}

static void
OnStreamChanged(OH_AVCodec* codec, OH_AVFormat* format, void* userData) {
    (void)codec;
    (void)userData;

    int32_t width;
    int32_t height;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &width);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &height);

    LLogI(
        "OnStreamChanged received: curWidth: %{public}d, curHeight: %{public}d",
        width,
        height);
}

static inline void OnNeedInputBuffer(
    OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer, void* userData) {
    (void)codec;
    (void)userData;
    (void)index;
    (void)buffer;
}

static void OnNewOutputBuffer(
    OH_AVCodec* codec, uint32_t index, OH_AVBuffer* buffer, void* userData) {
    LiveEncoder* liveEncoder = (LiveEncoder*)userData;
    bool needFlushAndRequestIDR = false;
    
    OH_AVCodecBufferAttr attr;
    int32_t ret = OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if (ret != AV_ERR_OK) {
        LLogE("OH_AVBuffer_GetBufferAttr Failed, Reason: %{public}d", ret);
        OH_VideoEncoder_FreeOutputBuffer(codec, index);
        return;
    }
    uint8_t* addr = OH_AVBuffer_GetAddr(buffer);
    if (!addr) {
        LLogE("OH_AVBuffer_GetAddr Failed.");
        OH_VideoEncoder_FreeOutputBuffer(codec, index);
        return;
    }
    if ((attr.flags & AVCODEC_BUFFER_FLAGS_CODEC_DATA) != 0) {
        LLogI("Received codec data.");
    }
    if ((attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) != 0) {
        LLogI("Received I-Frame.");
    }
    if ((attr.flags & AVCODEC_BUFFER_FLAGS_INCOMPLETE_FRAME) != 0) {
        LLogW("Incomplete frame.");
        OH_VideoEncoder_FreeOutputBuffer(codec, index);
        return;
    }
    std::vector<std::vector<uint8_t>> nalUnits;
    LiveStreamNALUnitsExtract(
        std::span<const uint8_t>(addr, attr.size), nalUnits);

    auto timestampsUs =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (auto&& nal : nalUnits) {
        LiveNalType nalType = (LiveNalType)LiveStreamNalTypeRetrieve(nal[0]);
        if (nalType == LIVE_SPS_TYPE && liveEncoder->encSPS.empty()) {
            liveEncoder->encSPS = nal;
            LLogI(
                "Dump SPS: %{public}s",
                LiveStreamCodecExtraDataDump(liveEncoder->encSPS).c_str());
        }
        if (nalType == LIVE_PPS_TYPE && liveEncoder->encPPS.empty()) {
            liveEncoder->encPPS = nal;
            LLogI(
                "Dump PPS: %{public}s",
                LiveStreamCodecExtraDataDump(liveEncoder->encPPS).c_str());
        }
        needFlushAndRequestIDR = 
            LiveNalUnitQueueEnqueue(liveEncoder->encNalQueue,std::move(nal),timestampsUs,true);
        liveEncoder->encFrameAvailableCb();
    }
    if (liveEncoder->enableStreamDumper) {
        LiveStreamDumperDump(liveEncoder->streamDumper, (char*)addr, attr.size);
    }
    if (needFlushAndRequestIDR) {
        LLogW("The nalu queue has too much latency. To ensure real-time "
              "performance, we need to clear the queue.");
        
        LiveNalUnitQueueFlush(liveEncoder->encNalQueue);
        LiveEncoderRequestIDRFrame(liveEncoder);
    }
    OH_VideoEncoder_FreeOutputBuffer(codec, index);
}

void LiveEncoderDestroy(LiveEncoder* liveEncoder) {
    LLogI("%{public}s enter.", __FUNCTION__);
    if (liveEncoder->nativeWindow) {
        OH_NativeWindow_DestroyNativeWindow(liveEncoder->nativeWindow);
    }
    if (liveEncoder->encCore) {
        OH_VideoEncoder_NotifyEndOfStream(liveEncoder->encCore);
        OH_VideoEncoder_Destroy(liveEncoder->encCore);
    }
    if (liveEncoder->streamDumper) {
        LiveStreamDumperDestroy(liveEncoder->streamDumper);
    }
    if (liveEncoder->encNalQueue) {
        LiveNalUnitQueueDestroy(liveEncoder->encNalQueue);
    }
    if (liveEncoder) {
        delete liveEncoder;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}

LiveEncoder* LiveEncoderCreate(EncoderType encoderType) {
    LLogI("%{public}s enter.", __FUNCTION__);

    LiveEncoder* liveEncoder = new LiveEncoder;
    if (!liveEncoder) {
        return nullptr;
    }
    OH_AVCapability* capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    if (!capability) {
        LLogE("OH_AVCodec_GetCapabilityByCategory: Hardware encoder is not "
              "supported.");
        LiveEncoderDestroy(liveEncoder);
        return nullptr;
    }
    liveEncoder->enableLowLatency =
        OH_AVCapability_IsFeatureSupported(capability, VIDEO_LOW_LATENCY);
    liveEncoder->encType = encoderType;
    liveEncoder->encName = OH_AVCapability_GetName(capability);
    liveEncoder->encCore = OH_VideoEncoder_CreateByName(liveEncoder->encName);
    liveEncoder->encNalQueue = LiveNalUnitQueueCreate();

    liveEncoder->encCb.onError = OnError;
    liveEncoder->encCb.onStreamChanged = OnStreamChanged;
    liveEncoder->encCb.onNeedInputBuffer = OnNeedInputBuffer;
    liveEncoder->encCb.onNewOutputBuffer = OnNewOutputBuffer;

    if (liveEncoder->enableStreamDumper) {
        liveEncoder->streamDumper =
            LiveStreamDumperCreate(LIVE_ENCODER_DUMP_FILE);
    }

    int32_t ret = OH_VideoEncoder_RegisterCallback(
        liveEncoder->encCore, liveEncoder->encCb, liveEncoder);
    if (ret != AV_ERR_OK) {
        LLogE(
            "OH_VideoEncoder_RegisterCallback Failed, Reason: %{public}d.",
            ret);
        LiveEncoderDestroy(liveEncoder);
        return nullptr;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
    return liveEncoder;
}

void LiveEncoderConfigure(LiveEncoder* liveEncoder, LiveEncoderConfig& config) {
    LLogI("%{public}s enter.", __FUNCTION__);

    OH_AVFormat* format = OH_AVFormat_Create();
    /**
     * TODO:
     * Due to some parameters not supporting query value ranges, we will
     * unify the modifications once full support is available.
     */
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, config.iFrameInterval);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, config.width);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, config.height);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, config.pixelFmt);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, config.frameRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, config.encProfile);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, config.targetBitrate);
    OH_AVFormat_SetIntValue(
        format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, config.rateMode);
    if (liveEncoder->enableLowLatency) {
        LLogI("Support Low latency mode.");
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY, 1);
    }
    OH_AVErrCode ret = OH_VideoEncoder_Configure(liveEncoder->encCore, format);
    if (ret != AV_ERR_OK) {
        LLogE("OH_VideoEncoder_Configure Failed, Reason: %{public}d", ret);
    }
    OH_AVFormat_Destroy(format);

    liveEncoder->encTargetBitrate = config.targetBitrate;

    ret = OH_VideoEncoder_GetSurface(
        liveEncoder->encCore, &liveEncoder->nativeWindow);
    if (ret != AV_ERR_OK) {
        LLogE("OH_VideoEncoder_GetSurface Failed, Reason: %{public}d", ret);
    }
    OH_NativeWindow_NativeWindowHandleOpt(
        liveEncoder->nativeWindow,
        SET_BUFFER_GEOMETRY,
        liveEncoder->config.width,
        liveEncoder->config.height);

    ret = OH_VideoEncoder_Prepare(liveEncoder->encCore);
    if (ret != AV_ERR_OK) {
        LLogE("OH_VideoEncoder_Prepare Failed, Reason: %{public}d", ret);
    }
    LLogI("%{public}s leave.", __FUNCTION__);
    return;
}

void LiveEncoderStart(LiveEncoder* liveEncoder) {
    LLogI("%{public}s enter.", __FUNCTION__);

    OH_AVErrCode ret = OH_VideoEncoder_Start(liveEncoder->encCore);
    if (ret != AV_ERR_OK) {
        LLogE("OH_VideoEncoder_Start Failed, Reason: %{public}d", ret);
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}

void LiveEncoderRequestIDRFrame(LiveEncoder* liveEncoder) {
    LLogI("LiveEncoderRequestIFrame trigger.");

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, true);
    OH_VideoEncoder_SetParameter(liveEncoder->encCore, format);
    OH_AVFormat_Destroy(format);
}

std::string LiveEncoderSurfaceIdRetrieve(LiveEncoder* liveEncoder) {
    uint64_t surfaceId;
    OH_NativeWindow_GetSurfaceId(liveEncoder->nativeWindow, &surfaceId);
    
    LLogI("retrieved surfaceId:  %{public}" PRIu64 "", surfaceId);
    return std::to_string(surfaceId);
}

void LiveEncoderSetFrameAvailableCallback(
    LiveEncoder* liveEncoder, std::function<void()>& frameAvailableCb) {
    liveEncoder->encFrameAvailableCb = frameAvailableCb;
}

LiveNalUnitQueue* LiveEncoderFrameQueueRetrieve(LiveEncoder* liveEncoder) {
    return liveEncoder->encNalQueue;
}

int64_t LiveEncoderTargetBitrateRetrieve(LiveEncoder* liveEncoder) {
    return liveEncoder->encTargetBitrate;
}

std::vector<uint8_t> LiveEncoderSPSRetrieve(LiveEncoder* liveEncoder) {
    return liveEncoder->encSPS;
}

std::vector<uint8_t> LiveEncoderPPSRetrieve(LiveEncoder* liveEncoder) {
    return liveEncoder->encPPS;
}

std::vector<uint8_t> LiveEncoderSEIRetrieve(LiveEncoder* liveEncoder) {
    return liveEncoder->encSEI;
}
