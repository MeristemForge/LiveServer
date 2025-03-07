# Copyright (c) 2025, Archermind Tech. Co., Ltd. <marketing@archermind.com>
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to
#  deal in the Software without restriction, including without limitation the
#  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
#  sell copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
#  IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.5.0)
project(LiveServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/live555/include/BasicUsageEnvironment
                    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/live555/include/groupsock
                    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/live555/include/liveMedia
                    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/live555/include/UsageEnvironment)

add_library(BasicUsageEnvironment STATIC IMPORTED)
set_target_properties(BasicUsageEnvironment
    PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third-party/live555/libs/${OHOS_ARCH}/libBasicUsageEnvironment.a)

add_library(groupsock STATIC IMPORTED)
set_target_properties(groupsock
    PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third-party/live555/libs/${OHOS_ARCH}/libgroupsock.a)

add_library(liveMedia STATIC IMPORTED)
set_target_properties(liveMedia
    PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third-party/live555/libs/${OHOS_ARCH}/libliveMedia.a)

add_library(UsageEnvironment STATIC IMPORTED)
set_target_properties(UsageEnvironment
    PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third-party/live555/libs/${OHOS_ARCH}/libUsageEnvironment.a)

set(SRCS LiveServerNapi.cpp
         LiveCamera.cpp
         LiveEncoder.cpp
         LiveWaitGroup.cpp
         LiveDeviceSource.cpp
         LiveServerMediaSubsession.cpp
         LiveDevice.cpp
         LiveDeviceManager.cpp
         LiveServer.cpp
         LiveUtils.cpp
         LiveNalUnitQueue.cpp
         LivePreProcessor.cpp
)
add_library(entry SHARED ${SRCS})
target_link_libraries(
        entry PUBLIC libace_napi.z.so
                     libhilog_ndk.z.so
                     libohcamera.so
                     libnative_window.so
                     libnative_media_codecbase.so
                     libnative_media_core.so
                     libnative_media_venc.so
                     libEGL.so libGLESv3.so
                     libnative_image.so
                     libnative_buffer.so
                     BasicUsageEnvironment
                     groupsock
                     liveMedia
                     UsageEnvironment
)