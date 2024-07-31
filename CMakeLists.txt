cmake_minimum_required(VERSION 3.10)
project(StreamingGatewayCrow)

# Compiler options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc")

# Boost linking
find_package(
        Boost REQUIRED COMPONENTS
        system thread log
)
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



add_library(
        shared_memory_manager SHARED

        src/shared_memory_manager.cpp
        src/shared_memory_manager.h
)

add_library(
        common SHARED

        src/HandleStreamDaemon.cpp
        src/ipc_message_queue.cpp
        src/ipc_message_queue.h
        src/cli.cpp
        src/exporters.cpp
        src/exporters.h
        src/constants.h
        src/utils.h
        src/utils.cpp
        src/logging.h
        src/logging.cpp
)
target_link_libraries(common ${OpenCV_LIBS} ${Boost_LIBRARIES} Boost::log -lrt -lpthread -ldl)

add_library(
        webcam SHARED

        src/WebCam.cpp
        src/WebCam.h
        src/wc_daemon.cpp
        src/wc_daemon.h
)
target_link_libraries(webcam common)

# executables and libs
add_executable(
        flowweave

        src/main.cpp
        src/cli.cpp
        src/http.cpp
)


add_executable(
        video_streamer

        src/webcam_streamer.cpp   # i am redundant (for example)
)

add_executable(
        tcp_streamer

        TCPStreamer/TCPStreamer.cpp
        TCPStreamer/TCPStreamer.h
        TCPStreamer/main.cpp
        src/wc_daemon.cpp
        src/wc_daemon.h
)

add_executable(
        video_handler

        src/WebCam.cpp
        src/WebCam.h
        src/wc_daemon.cpp
        src/wc_daemon.h
        src/webcam_handler.cpp
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
target_compile_options(flowweave PUBLIC -g)
target_link_options(flowweave PUBLIC -g)
target_compile_options(shared_memory_manager PUBLIC -g)
target_link_options(shared_memory_manager PUBLIC -g)

target_link_options(video_handler PUBLIC -g)
target_compile_options(video_handler PUBLIC -g)

target_compile_options(tcp_streamer PUBLIC -g)

# linking
target_link_libraries(flowweave shared_memory_manager common)
target_link_libraries(shared_memory_manager common)

target_link_libraries(video_streamer shared_memory_manager common webcam)

target_link_libraries(video_handler shared_memory_manager common webcam)


find_package(ZLIB REQUIRED)
target_link_libraries(tcp_streamer shared_memory_manager common ${ZLIB_LIBRARIES})
target_link_libraries(tcp_producer shared_memory_manager ${OpenCV_LIBS} ${Boost_LIBRARIES} Boost::log ${ZLIB_LIBRARIES} -lrt -lpthread -ldl)


# Option to enable or disable building of tests
set(BUILD_TESTS ON CACHE BOOL "Build the unit tests." FORCE)

# Conditionally compiling the test executable if BUILD_TESTS is set to ON
if(BUILD_TESTS)
    # Finding Google Test framework
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src/)
    add_library(tests SHARED qa/main.cpp)
    target_link_libraries(tests shared_memory_manager ${GTEST_LIBRARIES} GTest::GTest GTest::gmock common)

    # shm testing
    add_executable(tests_shm_manager
            qa/test_shared_memory_manager/test_shared_memory_manager.cpp
    )
    target_link_libraries(tests_shm_manager PUBLIC tests)
    # webcam streamer tester
    add_executable(tests_webcam
            qa/test_webcam_streamer_queue/test_webcam_streamer_queue.cpp
    )
    target_link_libraries(tests_webcam PUBLIC tests webcam)
    # all tests
    add_executable(test_all
            qa/main.cpp
            qa/test_shared_memory_manager/test_shared_memory_manager.cpp
            qa/test_webcam_streamer_queue/test_webcam_streamer_queue.cpp
    )
    target_link_libraries(test_all PUBLIC tests webcam)
    # Discover and run the Google Tests
    gtest_discover_tests(test_all)
endif()

