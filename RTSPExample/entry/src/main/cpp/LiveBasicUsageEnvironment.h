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

#define NO_OPENSSL

#include "LiveLog.h"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

class LiveUsageEnvironment : public BasicUsageEnvironment {
  public:
    static LiveUsageEnvironment* createNew(TaskScheduler& scheduler) {
        return new LiveUsageEnvironment(scheduler);
    }

  protected:
    LiveUsageEnvironment(TaskScheduler& scheduler)
        : BasicUsageEnvironment(scheduler) {
    }
    virtual UsageEnvironment& operator<<(char const* str) override {
        LLogI("%{public}s", str);
        return *this;
    }
    virtual UsageEnvironment& operator<<(int i) override {
        LLogI("%{public}d", i);
        return *this;
    }
    virtual UsageEnvironment& operator<<(unsigned u) override {
        LLogI("%{public}u", u);
        return *this;
    }
    virtual UsageEnvironment& operator<<(double d) override {
        LLogI("%{public}f", d);
        return *this;
    }
    virtual UsageEnvironment& operator<<(void* p) override {
        LLogI("%{public}p", p);
        return *this;
    }
};
