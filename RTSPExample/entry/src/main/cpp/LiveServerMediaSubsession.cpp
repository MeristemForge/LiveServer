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
#include "LiveServerMediaSubsession.h"

#define KILO 1000

LiveServerMediaSubsession* LiveServerMediaSubsession::createNew(
    UsageEnvironment& env,
    StreamReplicator* streamReplica,
    LiveEncoder*      liveEncoder) {
    return new LiveServerMediaSubsession(env, streamReplica, liveEncoder);
}

LiveServerMediaSubsession::LiveServerMediaSubsession(
    UsageEnvironment& env,
    StreamReplicator* streamReplica,
    LiveEncoder*      liveEncoder)
    : OnDemandServerMediaSubsession(env, false),
      fStreamReplicator(streamReplica), fLiveEncoder(liveEncoder) {
    LLogI("%{public}s enter.", __FUNCTION__);
    fSPS = LiveEncoderSPSRetrieve(fLiveEncoder).data();
    fSPSLen = LiveEncoderSPSRetrieve(fLiveEncoder).size();

    fPPS = LiveEncoderPPSRetrieve(fLiveEncoder).data();
    fPPSLen = LiveEncoderPPSRetrieve(fLiveEncoder).size();
    LLogI("%{public}s leave.", __FUNCTION__);
}

LiveServerMediaSubsession::~LiveServerMediaSubsession() {
    LLogI("%{public}s enter.", __FUNCTION__);
    LLogI("%{public}s leave.", __FUNCTION__);
}

FramedSource* LiveServerMediaSubsession::createNewStreamSource(
    unsigned clientSessionId, unsigned& estBitrate) {
    estBitrate = (uint32_t)(LiveEncoderTargetBitrateRetrieve(fLiveEncoder) /
                            KILO); // kbps
    return H264VideoStreamDiscreteFramer::createNew(
        envir(), fStreamReplicator->createStreamReplica());
}

RTPSink* LiveServerMediaSubsession::createNewRTPSink(
    Groupsock*    rtpGroupsock,
    unsigned char rtpPayloadTypeIfDynamic,
    FramedSource* inputSource) {
    LiveEncoderRequestIDRFrame(fLiveEncoder);
//     return H264VideoRTPSink::createNew(
//         envir(),
//         rtpGroupsock,
//         rtpPayloadTypeIfDynamic,
//         fSPS,
//         fSPSLen,
//         fPPS,
//         fPPSLen);
    return H264VideoRTPSink::createNew(
        envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
