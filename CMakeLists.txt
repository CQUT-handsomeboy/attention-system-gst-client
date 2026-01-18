cmake_minimum_required(VERSION 3.13)

project(attention-system-rknn-client LANGUAGES C CXX)

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
