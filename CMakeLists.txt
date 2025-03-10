cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

project(FloWWeaver_managed)  # raw FloWWeaver - in core/

add_subdirectory(core)
add_subdirectory(webcam)
add_subdirectory(TCPStreamer)

include_directories(core)
include_directories(webcam)

# Option to enable or disable building of tests
set(BUILD_TESTS ON CACHE BOOL "Build the unit tests." FORCE)

# Conditionally compiling the test executable if BUILD_TESTS is set to ON
if(BUILD_TESTS)
    # Finding Google Test framework
    find_package(GTest REQUIRED)
    include_directories(core/src)
    include_directories(core/common/src)
    include_directories(webcam/src)

    add_library(tests SHARED qa/main.cpp)
    target_link_libraries(tests webcam common ${GTEST_LIBRARIES} GTest::GTest GTest::gmock)

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

