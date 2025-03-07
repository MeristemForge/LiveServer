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
#include "LiveBasicUsageEnvironment.h"
#include "liveMedia.hh"
#include "GroupsockHelper.hh"
#include "LiveLog.h"

UsageEnvironment* env;
Boolean           reuseFirstSource = False;
Boolean           iFramesOnly = False;

static void announceStream(
    RTSPServer*         rtspServer,
    ServerMediaSession* sms,
    char const*         streamName,
    char const*         inputFileName); // forward

void announceURL(RTSPServer* rtspServer, ServerMediaSession* sms) {
    if (rtspServer == NULL || sms == NULL)
        return; // sanity check

    UsageEnvironment& env = rtspServer->envir();
    if (weHaveAnIPv4Address(env)) {
        char* url = rtspServer->ipv4rtspURL(sms);
        LLogI("Play this stream using the URL %{public}s", url);
        delete[] url;
    }
    if (weHaveAnIPv6Address(env)) {
        char* url = rtspServer->ipv6rtspURL(sms);
        LLogI("Play this stream using the URL %{public}s", url);
        delete[] url;
    }
}

void livingServerTest(void) {
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = LiveUsageEnvironment::createNew(*scheduler);

    RTSPServer* rtspServer = RTSPServer::createNew(*env, 18554, NULL);
    if (rtspServer == NULL) {
        LLogE("Failed to create RTSP server: %{public}s", env->getResultMsg());
        exit(1);
    }
    char const* descriptionString = "Session streamed by livingServerTest";
    {
        char const* streamName = "h264ESVideoTest";
        char const* inputFileName = "/data/storage/el2/base/files/test.h264";
        ServerMediaSession* sms = ServerMediaSession::createNew(
            *env, streamName, streamName, descriptionString);
        sms->addSubsession(H264VideoFileServerMediaSubsession ::createNew(
            *env, inputFileName, reuseFirstSource));
        rtspServer->addServerMediaSession(sms);

        announceStream(rtspServer, sms, streamName, inputFileName);
    }
    env->taskScheduler().doEventLoop();
    return;
}

static void announceStream(
    RTSPServer*         rtspServer,
    ServerMediaSession* sms,
    char const*         streamName,
    char const*         inputFileName) {
    LLogI(
        "%{public}s stream, from the file %{public}s",
        streamName,
        inputFileName);
    announceURL(rtspServer, sms);
}
