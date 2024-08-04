cmake_minimum_required(VERSION 3.10)

add_executable(
        tcp_streamer

        src/TCPStreamer.cpp
        src/TCPStreamer.h
        src/main.cpp
)

add_executable(
        tcp_producer

        src/tcp_producing.cpp
        src/tcp_producing.h
        src/TCPPublisher.h
        src/tcp_frame_producer_main.cpp
)

# flags for libs

target_compile_options(tcp_streamer PUBLIC -g)

find_package(ZLIB REQUIRED)
target_link_libraries(tcp_streamer webcam shared_memory_manager common ${ZLIB_LIBRARIES})
target_link_libraries(tcp_producer webcam shared_memory_manager common ${OpenCV_LIBS} ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} -lrt -lpthread -ldl)
