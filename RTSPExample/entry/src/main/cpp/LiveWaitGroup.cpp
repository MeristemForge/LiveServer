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
#include <mutex>
#include <condition_variable>

struct LiveWaitGroup {
    int                     cnt;
    std::mutex              mtx;
    std::condition_variable cnd;
};

LiveWaitGroup* LiveWaitGroupCreate(void) {
    LiveWaitGroup* wg = new LiveWaitGroup;
    if (!wg) {
        return nullptr;
    }
    wg->cnt = 0;
    return wg;
}

void LiveWaitGroupDestroy(LiveWaitGroup* wg) {
    if (wg) {
        wg->cnt = 0;
        delete wg;
    }
}

void LiveWaitGroupAdd(LiveWaitGroup* wg, int delta) {
    std::lock_guard<std::mutex> lock(wg->mtx);
    wg->cnt += delta;
}

void LiveWaitGroupDone(LiveWaitGroup* wg) {
    std::lock_guard<std::mutex> lock(wg->mtx);
    wg->cnt--;
    if (wg->cnt == 0) {
        wg->cnd.notify_all();
    }
}

void LiveWaitGroupWait(LiveWaitGroup* wg) {
    std::unique_lock<std::mutex> lock(wg->mtx);
    wg->cnd.wait(lock, [wg] { return wg->cnt == 0; });
}