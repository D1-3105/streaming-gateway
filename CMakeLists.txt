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

# Opencv
find_package(OpenCV REQUIRED)
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
add_executable(streaming_gateway_crow src/main.cpp src/cli.cpp src/http.cpp src/logging.h src/shared_memory_manager.cpp src/shared_memory_manager.h
        src/webcam_streamer.cpp
        src/ipc_message_queue.cpp
        src/ipc_message_queue.h
)

add_library(shared_memory_manager SHARED src/shared_memory_manager.cpp src/shared_memory_manager.h)

add_executable(tests src/logging.h
        src/shared_memory_manager.cpp
        src/ipc_message_queue.cpp
        qa/qa_main.cpp
        qa/test_shared_memory_manager.hpp
        qa/BaseTest.h
        qa/test_webcam_streamer_queue.hpp
        src/WebCam.cpp
        src/WebCam.cpp
)

add_executable(video_streamer src/HandleStreamDaemon.cpp src/webcam_streamer.cpp
        src/WebCam.h
        src/ipc_message_queue.cpp
        src/ipc_message_queue.h
        src/WebCam.cpp
)

# flags for libs
target_compile_options(streaming_gateway_crow PUBLIC -g)
target_link_options(streaming_gateway_crow PUBLIC -g)
target_compile_options(shared_memory_manager PUBLIC -g)
target_link_options(shared_memory_manager PUBLIC -g)
target_link_options(tests PUBLIC -g)
target_compile_options(tests PUBLIC -g)

# linking
target_link_libraries(streaming_gateway_crow ${Boost_LIBRARIES} ${OpenCV_LIBS} -lrt -lpthread)
target_link_libraries(shared_memory_manager ${Boost_LIBRARIES} Boost::log -lpthread)
target_link_libraries(tests shared_memory_manager ${Boost_LIBRARIES} ${OpenCV_LIBS} Boost::log -lrt -lpthread)
target_link_libraries(video_streamer shared_memory_manager ${OpenCV_LIBS} ${Boost_LIBRARIES} Boost::log -lrt -lpthread)
