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
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>

#include "LiveLog.h"
#include "LiveUtils.h"

struct LiveStreamDumper {
    std::ofstream file;
};

LiveStreamDumper* LiveStreamDumperCreate(std::string filename) {
    LiveStreamDumper* streamDumper = new LiveStreamDumper;
    if (!streamDumper) {
        return nullptr;
    }
    streamDumper->file.open(filename, std::ios::binary);
    if (!streamDumper->file.is_open()) {
        LLogE("Open %{public}s Failed.", filename.c_str());
        LiveStreamDumperDestroy(streamDumper);
        return nullptr;
    }
    return streamDumper;
}

void LiveStreamDumperDump(
    LiveStreamDumper* streamDumper, char* buf, uint32_t len) {
    streamDumper->file.write(buf, len);
    if (!streamDumper->file.good()) {
        LLogE("Write Failed.");
    }
}

void LiveStreamDumperDestroy(LiveStreamDumper* streamDumper) {
    if (streamDumper->file.is_open()) {
        streamDumper->file.close();
    }
    if (streamDumper) {
        delete streamDumper;
    }
}

bool LiveStreamStartCodeSearch(
    const uint8_t* data, int size, int& startCodeSize) {
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 &&
        data[3] == 0x01) {
        startCodeSize = 4;
        return true;
    }
    if (size >= 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
        startCodeSize = 3;
        return true;
    }
    return false;
}

void LiveStreamNALUnitsExtract(std::span<const uint8_t> h264Stream, std::vector<std::vector<uint8_t>>& nalUnits) {
    size_t pos = 0;
    const size_t streamSize = h264Stream.size();
    while (pos < streamSize) {
        int startCodeSize = 0;
        if (pos + 3 > streamSize ||
            !LiveStreamStartCodeSearch(
                &h264Stream[pos], streamSize - pos, startCodeSize)) {
            pos++;
            continue;
        }
        const size_t nalStart = pos + startCodeSize;
        pos = nalStart;
        size_t nextStart = streamSize;
        for (size_t i = nalStart; i < streamSize; ++i) {
            if (i + 3 > streamSize) {
                break;
            }
            int codeSize = 0;
            if (LiveStreamStartCodeSearch(
                    &h264Stream[i], streamSize - i, codeSize)) {
                nextStart = i;
                break;
            }
        }
        if (nextStart >= nalStart) {
            const size_t nalEnd = nextStart;
            if (nalEnd > nalStart) {
                nalUnits.emplace_back(
                    h264Stream.begin() + nalStart, h264Stream.begin() + nalEnd);
            }
        }
        pos = nextStart;
    }
}

int LiveStreamNalTypeRetrieve(uint8_t nalHeader) {
    uint8_t nalType = nalHeader & 0x1F;
    return nalType;
}

std::string LiveStreamCodecExtraDataDump(const std::vector<uint8_t>& nal) {
    std::ostringstream oss;
    for (const auto& byte : nal) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte) << " ";
    }
    return oss.str();
}