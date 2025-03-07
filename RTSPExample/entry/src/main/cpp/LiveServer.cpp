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
#include <thread>

#include "LiveServer.h"
#include "LiveDevice.h"
#include "LiveWaitGroup.h"
#include "LiveDeviceManager.h"
#include "LiveServerMediaSubsession.h"
#include "LiveBasicUsageEnvironment.h"

#define MAX_OUT_PKG_SIZE (1024 * 1024)
#define MAX_RECLAMATION_TIME 10 //10s

LiveServer                    liveServer;
static std::atomic<bool>      started;
static EventLoopWatchVariable eventLoopWatchVariable;

static inline void _LiveServerPray(void){
    std::string fozu = R"(
                            _ooOoo_
                           o8888888o
                           88" . "88
                           (| -_- |)
                            O\ = /O
                        ____/`---'\____
                      .   ' \\| |// `.
                       / \\||| : |||// \
                     / _||||| -:- |||||- \
                       | | \\\ - /// | |
                     | \_| ''\---/'' | |
                      \ .-\__ `-` ___/-. /
                   ___`. .' /--.--\ `. . __
                ."" '< `.___\_<|>_/___.' >'"".
               | | : `- \`.;`\ _ /`;.`/ - ` : | |
                 \ \ `-. \_ __\ /__ _/ .-` / /
         ======`-.____`-.___\_____/___.-`____.-'======
                            `=---='
    )";
    LLogW("%{public}s", fozu.c_str());
}

static inline void _LiveServerRundown(void) {
    LLogI("%{public}s enter.", __FUNCTION__);
    
    for(auto stm : liveServer.liveStreams){
        LiveDeviceDestroy(stm.second.device);
    }
    if (liveServer.liveDM) {
        LiveDeviceManagerDestroy(liveServer.liveDM);
    }
    if (liveServer.rtspServer) {
        Medium::close(liveServer.rtspServer);
    }
    if (liveServer.env) {
        liveServer.env->reclaim();
    }
    if (liveServer.scheduler) {
        delete liveServer.scheduler;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
}

static void _LiveServerRun(LiveWaitGroup* wg) {
    liveServer.scheduler = BasicTaskScheduler::createNew();
    liveServer.env = LiveUsageEnvironment::createNew(*liveServer.scheduler);

    liveServer.rtspServer = RTSPServer::createNew(*liveServer.env, liveServer.serverPort, NULL, MAX_RECLAMATION_TIME);
    if (!liveServer.rtspServer) {
        LLogE(
            "Failed to create Live server: %{public}s",
            liveServer.env->getResultMsg());

        LiveWaitGroupDone(wg);
        _LiveServerRundown();
        started = false;
        return;
    }
    OutPacketBuffer::maxSize = MAX_OUT_PKG_SIZE;
    
    liveServer.liveDM = LiveDeviceManagerCreate(LIVE_CAMERA_TYPE);

    for (auto& di : LiveDeviceManagerDevicesInfoRetrieve(liveServer.liveDM)) {
        LLogI(
            "{devNo: %{public}d, devName: %{public}s, devType: %{public}s}, ",
            di.devNo,
            di.devName,
            di.devType == LIVE_CAMERA_TYPE ? "Camera" : "Unknown");
        {
            LiveStream stm;
            stm.device = LiveDeviceCreate(di);
            stm.deviceSource = LiveDeviceSource::createNew(*liveServer.env, stm.device);
            stm.replicator = StreamReplicator::createNew(*liveServer.env, stm.deviceSource, false);
            stm.smsSession = ServerMediaSession::createNew(*liveServer.env, di.devName, di.devName);

            stm.smsSession->addSubsession(LiveServerMediaSubsession::createNew(
                *liveServer.env,
                stm.replicator,
                LiveDeviceEncoderRetrieve(stm.device)));
            
            liveServer.rtspServer->addServerMediaSession(stm.smsSession);
            
            char* url = liveServer.rtspServer->rtspURL(stm.smsSession);
            stm.publishUrl = std::string(url);
            delete[] url;
            
            if (stm.deviceSource && stm.device && stm.smsSession && !stm.publishUrl.empty()){
                liveServer.liveStreams[di.devNo] = stm;
            }
        }
    }
    LLogI("PUBLISH RTSP URLs:");
    for(auto stm : liveServer.liveStreams){
        LLogI("\t %{public}s", stm.second.publishUrl.c_str());
    }
    LiveWaitGroupDone(wg);
    liveServer.env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    _LiveServerRundown();
    started = false;
    eventLoopWatchVariable = 0;
}

std::vector<std::string> LiveServerStart(void) {
    if (started) {
        LLogI("The LIVE-SERVER is Already Running.");
        return {};
    }
    LLogI("Starting the LIVE-SERVER.");
    started = true;
    LiveWaitGroup* wg = LiveWaitGroupCreate();
    LiveWaitGroupAdd(wg, 1);
    
    std::thread(_LiveServerRun, wg).detach();

    LiveWaitGroupWait(wg);
    LiveWaitGroupDestroy(wg);
    
    LLogI("The LIVE-SERVER is Running.");
    std::vector<std::string> urls;
    for(auto stm : liveServer.liveStreams){
        urls.emplace_back(stm.second.publishUrl);
    }
    _LiveServerPray();
    return urls;
}

void LiveServerStop(void) {
    if (!started) {
        LLogI("The LIVE-SERVER has Not-Started Yet.");
        return;
    }
    eventLoopWatchVariable = 1;
    LLogI("Stopping the LIVE-SERVER.");
}
