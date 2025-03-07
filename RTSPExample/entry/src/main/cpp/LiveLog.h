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

#include <hilog/log.h>

#undef LOG_DOMAIN
#define LOG_DOMAIN 0x1234

#undef LOG_TAG
#define LOG_TAG "LiveServer"

#define BASENAME(file) (strrchr(file, '/') ? strrchr(file, '/') + 1 : file)
#define BASENAME_WIN(file) (strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file)

#if defined(_WIN32)
    #define GET_BASENAME(file) BASENAME_WIN(file)
#else
    #define GET_BASENAME(file) BASENAME(file)
#endif

#define LIVE_SERVER_LOG(func, fmt, args...)                                    \
    do {                                                                       \
        (void)func(                                                            \
            LOG_APP,                                                           \
            "%{public}s:%{public}d " fmt,                                      \
            GET_BASENAME(__FILE__),                                            \
            __LINE__,                                                          \
            ##args);                                                           \
    } while (0)

#define LLogF(fmt, ...) LIVE_SERVER_LOG(OH_LOG_FATAL, fmt, ##__VA_ARGS__)
#define LLogE(fmt, ...) LIVE_SERVER_LOG(OH_LOG_ERROR, fmt, ##__VA_ARGS__)
#define LLogW(fmt, ...) LIVE_SERVER_LOG(OH_LOG_WARN, fmt, ##__VA_ARGS__)
#define LLogI(fmt, ...) LIVE_SERVER_LOG(OH_LOG_INFO, fmt, ##__VA_ARGS__)
#define LLogD(fmt, ...) LIVE_SERVER_LOG(OH_LOG_DEBUG, fmt, ##__VA_ARGS__)
