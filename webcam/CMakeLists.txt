cmake_minimum_required(VERSION 3.10)

add_library(
        webcam SHARED

        src/WebCam.cpp
        src/WebCam.h
        src/wc_daemon.cpp
        src/wc_daemon.h
)

target_link_libraries(webcam common)

add_executable(
        video_handler

        src/webcam_handler.cpp
)

add_executable(
        video_streamer

        src/webcam_streamer.cpp
)

target_link_libraries(video_handler webcam)
target_link_libraries(video_streamer webcam)

target_link_options(video_handler PUBLIC -g)
target_compile_options(video_handler PUBLIC -g)
