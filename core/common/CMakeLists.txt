cmake_minimum_required(VERSION 3.10)

# Opencv with FFmpeg
find_package(OpenCV REQUIRED)
set(OpenCV_FIND_QUIETLY FALSE)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

# Boost linking
find_package(
        Boost REQUIRED COMPONENTS
        system thread log
)
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

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

add_library(
        shared_memory_manager SHARED

        src/shared_memory_manager.cpp
        src/shared_memory_manager.h
)

target_include_directories(common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
        shared_memory_manager

        Boost::log
)

target_link_libraries(common
        shared_memory_manager
        ${OpenCV_LIBS}
        ${Boost_LIBRARIES}
        Boost::log
        -lrt
        -lpthread
        -ldl
)
