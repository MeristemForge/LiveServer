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

#include <mutex>
#include <deque>
#include <optional>

enum LiveNalType {
    LIVE_PPS_TYPE = 0x8,
    LIVE_SPS_TYPE = 0x7,
    LIVE_SEI_TYPE = 0x6,
    LIVE_SLICE_IDR_TYPE = 0x5,
};

struct LiveNalUnit {
    std::vector<uint8_t> nal;
    int64_t              timestampsUs;
};

struct LiveNalUnitQueue {
    std::mutex              mtx;
    std::deque<LiveNalUnit> nals;
};

extern LiveNalUnitQueue* LiveNalUnitQueueCreate(void);
extern void LiveNalUnitQueueDestroy(LiveNalUnitQueue* nalQueue);
extern void LiveNalUnitQueueFlush(LiveNalUnitQueue* nalQueue);
extern bool LiveNalUnitQueueEnqueue(LiveNalUnitQueue* nalQueue, std::vector<uint8_t>&& nal, int64_t timestampsUs, bool toTail);
extern std::optional<LiveNalUnit> LiveNalUnitQueueDequeue(LiveNalUnitQueue* nalQueue);