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
#include "LiveDeviceSource.h"
#include "LiveNalUnitQueue.h"
#include "LiveServer.h"
#include "LiveDevice.h"

LiveDeviceSource*
LiveDeviceSource::createNew(UsageEnvironment& env, LiveDevice* device) {
    return new LiveDeviceSource(env, device);
}

void LiveDeviceSource::frameAvailable() {
    envir().taskScheduler().triggerEvent(fEventTriggerId, this);
}

LiveDeviceSource::LiveDeviceSource(UsageEnvironment& env, LiveDevice* device)
    : FramedSource(env), fDevice(device) {
    LLogI("%{public}s enter.", __FUNCTION__);

    fEventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    std::function<void()> frameAvailableCb =
        std::bind(&LiveDeviceSource::frameAvailable, this);

    LiveEncoderSetFrameAvailableCallback(
        LiveDeviceEncoderRetrieve(fDevice), frameAvailableCb);

    fNalQueue =
        LiveEncoderFrameQueueRetrieve(LiveDeviceEncoderRetrieve(fDevice));

    LiveDeviceStart(fDevice);
    LLogI("%{public}s leave.", __FUNCTION__);
}

LiveDeviceSource::~LiveDeviceSource() {
    LLogI("%{public}s enter.", __FUNCTION__);
    envir().taskScheduler().deleteEventTrigger(fEventTriggerId);
    LLogI("%{public}s leave.", __FUNCTION__);
}

void LiveDeviceSource::deliverFrame0(void* clientData) {
    ((LiveDeviceSource*)clientData)->deliverFrame();
}

void LiveDeviceSource::deliverFrame() {
    if (!isCurrentlyAwaitingData()) {
        return;
    }
    std::optional<LiveNalUnit>&& nalUnit = LiveNalUnitQueueDequeue(fNalQueue);
    if (!nalUnit.has_value()) {
        return;
    }
    uint8_t* newFrameDataStart = nalUnit->nal.data();
    uint32_t newFrameSize = nalUnit->nal.size();
    int64_t  newFrameTimestamps = nalUnit->timestampsUs;
    /**
     * TODO:
     * The fMaxSize here represents the maximum size of the output buffer. If
     * we can control the RTP, then we can set fMaxSize to the MTU (Maximum
     * Transmission Unit) minus the sizes of the UDP and RTP headers.
     */
    if (newFrameSize > fMaxSize) {
        LLogW(
            "The Frame buffer has been truncated. newFrameSize: %{public}u, "
            "fMaxSize: %{public}u",
            newFrameSize,
            fMaxSize);

        fFrameSize = fMaxSize;
        fNumTruncatedBytes = newFrameSize - fMaxSize;
    } else {
        fFrameSize = newFrameSize;
    }
    fPresentationTime.tv_sec  = newFrameTimestamps / std::micro::den;
    fPresentationTime.tv_usec = newFrameTimestamps % std::micro::den;

    memmove(fTo, newFrameDataStart, fFrameSize);
    afterGetting(this);
}

void LiveDeviceSource::doGetNextFrame() {
    deliverFrame();
}

void LiveDeviceSource::doStopGettingFrames(){
    FramedSource::doStopGettingFrames();
}