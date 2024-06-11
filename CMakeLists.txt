cmake_minimum_required(VERSION 3.10)
project(StreamingGatewayCrow)

# Compiler options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc")

# Boost linking
find_package(Boost REQUIRED COMPONENTS system thread log)
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

# Opencv with FFmpeg
find_package(OpenCV REQUIRED)
set(OpenCV_FIND_QUIETLY FALSE)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

# Boost+Crow
include_directories(${Boost_INCLUDE_DIRS})
set(CROW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/crow")
include_directories(${CROW_INCLUDE_DIR})

# Boost.ASIO
add_definitions(-DCROW_USE_BOOST)
add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_BOOST_DATE_TIME_NO_LIB)
add_definitions(-DASIO_DISABLE_BOOST_ARRAY)
add_definitions(-DASIO_DISABLE_BOOST_BIND)
add_definitions(-DASIO_DISABLE_BOOST_DATE_TIME)
add_definitions(-DASIO_DISABLE_BOOST_REGEX)

# executables and libs
add_executable(streaming_gateway_crow src/main.cpp
        src/cli.cpp
        src/http.cpp
        src/logging.cpp
        src/logging.h
        src/shared_memory_manager.cpp
        src/shared_memory_manager.h
        src/ipc_message_queue.cpp
        src/ipc_message_queue.h
        src/exporters.cpp
        src/exporters.h
        src/constants.h
        src/utils.h
        src/utils.cpp
)

add_library(shared_memory_manager SHARED src/shared_memory_manager.cpp src/shared_memory_manager.h
)

add_executable(tests src/logging.cpp
        src/logging.h
        src/HandleStreamDaemon.cpp
        src/shared_memory_manager.cpp
        src/ipc_message_queue.cpp
        qa/qa_main.cpp
        qa/test_shared_memory_manager.hpp
        qa/BaseTest.h
        qa/test_webcam_streamer_queue.hpp
        src/WebCam.cpp
        src/WebCam.cpp
        src/exporters.cpp
        src/exporters.h
        src/constants.h
        src/utils.h
        src/utils.cpp
        src/wc_daemon.cpp
        src/wc_daemon.h
)

add_executable(
        video_streamer src/webcam_streamer.cpp   # i am redundant (for example)
        src/HandleStreamDaemon.cpp
        src/WebCam.h
        src/ipc_message_queue.cpp
        src/ipc_message_queue.h
        src/WebCam.cpp
        src/cli.cpp
        src/constants.h
        src/utils.h
        src/utils.cpp
        src/wc_daemon.cpp
        src/wc_daemon.h
)

add_executable(
        tcp_streamer

        TCPStreamer/TCPStreamer.cpp
        TCPStreamer/TCPStreamer.h
        TCPStreamer/main.cpp

        src/logging.cpp
        src/logging.h
        src/wc_daemon.cpp
        src/wc_daemon.h

        src/HandleStreamDaemon.cpp
        src/ipc_message_queue.cpp
        src/cli.cpp
        src/constants.h
        src/utils.h
        src/utils.cpp
)

add_executable(
        video_handler src/HandleStreamDaemon.cpp  # i am redundant (for example)
        src/HandleStreamDaemon.cpp
        src/WebCam.cpp
        src/WebCam.h

        src/wc_daemon.cpp
        src/wc_daemon.h
        src/logging.cpp
        src/logging.h

        src/ipc_message_queue.cpp
        src/ipc_message_queue.h
        src/cli.cpp
        src/webcam_handler.cpp
        src/exporters.cpp
        src/exporters.h
        src/constants.h
        src/utils.h
        src/utils.cpp
)

add_executable(
        tcp_producer

        src/WebCam.cpp
        src/WebCam.h
        src/logging.cpp
        src/logging.h
        src/constants.h

        TCPStreamer/tcp_producing.cpp
        TCPStreamer/tcp_producing.h
        TCPStreamer/TCPPublisher.h
        TCPStreamer/tcp_frame_producer_main.cpp
)

# flags for libs
target_compile_options(streaming_gateway_crow PUBLIC -g)
target_link_options(streaming_gateway_crow PUBLIC -g)
target_compile_options(shared_memory_manager PUBLIC -g)
target_link_options(shared_memory_manager PUBLIC -g)
target_link_options(tests PUBLIC -g)
target_compile_options(tests PUBLIC -g)

target_link_options(video_handler PUBLIC -g)
target_compile_options(video_handler PUBLIC -g)

target_compile_options(tcp_streamer PUBLIC -g)

# linking
target_link_libraries(streaming_gateway_crow ${Boost_LIBRARIES} ${OpenCV_LIBS} -lrt -lpthread)
target_link_libraries(shared_memory_manager ${Boost_LIBRARIES} Boost::log -lpthread)
target_link_libraries(tests shared_memory_manager ${Boost_LIBRARIES} ${OpenCV_LIBS} Boost::log -lrt -lpthread)
target_link_libraries(video_streamer shared_memory_manager ${OpenCV_LIBS} ${Boost_LIBRARIES} Boost::log -lrt -lpthread)

target_link_libraries(video_handler shared_memory_manager ${OpenCV_LIBS} ${Boost_LIBRARIES} Boost::log -lrt -lpthread)


find_package(ZLIB REQUIRED)
target_link_libraries(tcp_streamer shared_memory_manager ${OpenCV_LIBS} ${Boost_LIBRARIES} Boost::log ${ZLIB_LIBRARIES} -lrt -lpthread)
target_link_libraries(tcp_producer shared_memory_manager ${OpenCV_LIBS} ${Boost_LIBRARIES} Boost::log ${ZLIB_LIBRARIES} -lrt -lpthread)
