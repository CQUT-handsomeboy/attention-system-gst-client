cmake_minimum_required(VERSION 3.13)

project(attention-system-rknn-client LANGUAGES C CXX)

option(BUILD_FOR_ARM32 "Build for ARM 32-bit architecture" OFF)
option(BUILD_FOR_AMD64 "Build for AMD64/x86_64 architecture" OFF)

# Check if exactly one option is selected
if((BUILD_FOR_ARM32 AND BUILD_FOR_AMD64) OR (NOT BUILD_FOR_ARM32 AND NOT BUILD_FOR_AMD64))
    message(FATAL_ERROR "You must select exactly one architecture (BUILD_FOR_ARM32 or BUILD_FOR_AMD64).")
endif()

if(BUILD_FOR_AMD64)

set_target_properties( PROPERTIES
    CXX_STANDARD 17
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(app main.cpp)

# find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

if(NOT PKG_CONFIG_FOUND)
	message(STATUS "PKG CONFIG NOT FOUND")
	return()
endif()

# GStreamer Base
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)

# GStreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER_AUDIO REQUIRED gstreamer-audio-1.0)
pkg_check_modules(GSTREAMER_WEBRTC REQUIRED gstreamer-webrtc-1.0)
pkg_check_modules(GSTREAMER_SDP REQUIRED gstreamer-sdp-1.0)
pkg_check_modules(GSTREAMER_RTP REQUIRED gstreamer-rtp-1.0)

target_include_directories(app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external
	${GSTREAMER_INCLUDE_DIRS}
	${GSTREAMER_BASE_INCLUDE_DIRS}
	${GSTREAMER_VIDEO_INCLUDE_DIRS}
	${GSTREAMER_APP_INCLUDE_DIRS}
	${GSTREAMER_AUDIO_INCLUDE_DIRS}
	${GSTREAMER_WEBRTC_INCLUDE_DIRS}
	${GSTREAMER_SDP_INCLUDE_DIRS}
	${GSTREAMER_RTP_INCLUDE_DIRS}

	${GLIB_INCLUDE_DIRS}
	${GOBJECT_INCLUDE_DIRS}

	${OPENSSL_INCLUDE_DIRS}
)

target_link_libraries(app
	${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${GSTREAMER_AUDIO_LIBRARIES}
    ${GSTREAMER_WEBRTC_LIBRARIES}
    ${GSTREAMER_SDP_LIBRARIES}
    ${GSTREAMER_RTP_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GOBJECT_LIBRARIES}

	${OPENSSL_LIBRARIES}
)

elseif(BUILD_FOR_ARM32)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm32)
set(CMAKE_SYSROOT /home/chen/gcc-arm-11.2-2022.02-x86_64-arm-none-linux-gnueabihf/arm-none-linux-gnueabihf/libc)
list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")

set(CMAKE_C_COMPILER   ${CMAKE_SYSROOT}/../../bin/arm-none-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${CMAKE_SYSROOT}/../../bin/arm-none-linux-gnueabihf-g++)

find_package(OpenSSL REQUIRED)

add_executable(arm32-app
	arm32_test.cpp
)

target_include_directories(arm32-app PUBLIC
	${OPENSSL_INCLUDE_DIRS}
)

target_link_libraries(arm32-app
	${OPENSSL_LIBRARIES}
)

endif()
