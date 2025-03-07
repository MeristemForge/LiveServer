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

#include "LiveNalUnitQueue.h"
#include "LiveLog.h"
#include "LiveConfig.h"

LiveNalUnitQueue* LiveNalUnitQueueCreate(void) {
    LLogI("%{public}s enter.", __FUNCTION__);

    LiveNalUnitQueue* nalQueue = new LiveNalUnitQueue;
    if (!nalQueue) {
        return nullptr;
    }
    LLogI("%{public}s leave.", __FUNCTION__);
    return nalQueue;
}

void LiveNalUnitQueueDestroy(LiveNalUnitQueue* nalQueue) {
    LLogI("%{public}s trigger.", __FUNCTION__);

    if (nalQueue) {
        LiveNalUnitQueueFlush(nalQueue);
        delete nalQueue;
    }
}

void LiveNalUnitQueueFlush(LiveNalUnitQueue* nalQueue) {
    LLogI("%{public}s trigger.", __FUNCTION__);

    std::unique_lock<std::mutex> lock(nalQueue->mtx);
    std::deque<LiveNalUnit>().swap(nalQueue->nals);
}

bool LiveNalUnitQueueEnqueue(LiveNalUnitQueue* nalQueue, std::vector<uint8_t>&& nal, int64_t timestampsUs, bool toTail) {
    std::unique_lock<std::mutex> lock(nalQueue->mtx);
    if (toTail) {
        if (!nalQueue->nals.empty()) {
            if ((timestampsUs - nalQueue->nals.front().timestampsUs) >
                LIVE_ENCODER_NALU_QUEUE_LATENCY) {
                return true;
            }
        }
        nalQueue->nals.emplace_back(LiveNalUnit{std::move(nal), timestampsUs});
    } else {
        nalQueue->nals.emplace_front(LiveNalUnit{std::move(nal), timestampsUs});
    }
    return false;
}

std::optional<LiveNalUnit> LiveNalUnitQueueDequeue(LiveNalUnitQueue* nalQueue) {
    std::unique_lock<std::mutex> lock(nalQueue->mtx);
    if (nalQueue->nals.empty()) {
        return std::nullopt;
    }
    LiveNalUnit nal = std::move(nalQueue->nals.front());
    nalQueue->nals.pop_front();
    return nal;
}