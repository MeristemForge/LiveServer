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

#include <string>
#include "LiveNalUnitQueue.h"

enum EncoderType{
    ENCODER_TYPE_H264
};

struct LiveEncoderConfig {
    uint32_t iFrameInterval;
    uint32_t width;
    uint32_t height;
    int32_t  pixelFmt;
    double   frameRate;
    int32_t  encProfile;
    int32_t  rateMode;
    int64_t  targetBitrate;
};

struct LiveEncoder;

extern LiveEncoder* LiveEncoderCreate(EncoderType encoderType);
extern void LiveEncoderDestroy(LiveEncoder* liveEncoder);
extern void LiveEncoderConfigure(LiveEncoder* liveEncoder, LiveEncoderConfig& config);
extern void LiveEncoderStart(LiveEncoder* liveEncoder);
extern void LiveEncoderRequestIDRFrame(LiveEncoder* liveEncoder);
extern std::string LiveEncoderSurfaceIdRetrieve(LiveEncoder* liveEncoder);
extern LiveNalUnitQueue* LiveEncoderFrameQueueRetrieve(LiveEncoder* liveEncoder);
extern int64_t LiveEncoderTargetBitrateRetrieve(LiveEncoder* liveEncoder);
extern std::vector<uint8_t> LiveEncoderSPSRetrieve(LiveEncoder* liveEncoder);
extern std::vector<uint8_t> LiveEncoderPPSRetrieve(LiveEncoder* liveEncoder);
extern std::vector<uint8_t> LiveEncoderSEIRetrieve(LiveEncoder* liveEncoder);
extern void LiveEncoderSetFrameAvailableCallback(LiveEncoder* liveEncoder, std::function<void()>& frameAvailableCb);