cmake_minimum_required(VERSION 3.28)
project(streaming_gateway_crow)

set(CMAKE_CXX_STANDARD 20)

# Find Boost with required components
find_package(Boost REQUIRED COMPONENTS system thread log)
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

# Include directories for Boost and Crow
include_directories(${Boost_INCLUDE_DIRS})
set(CROW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/crow")  # Ensure this is the correct path
include_directories(${CROW_INCLUDE_DIR})

# Define macros for Boost.ASIO
add_definitions(-DCROW_USE_BOOST)
add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_BOOST_DATE_TIME_NO_LIB)
add_definitions(-DASIO_DISABLE_BOOST_ARRAY)
add_definitions(-DASIO_DISABLE_BOOST_BIND)
add_definitions(-DASIO_DISABLE_BOOST_DATE_TIME)
add_definitions(-DASIO_DISABLE_BOOST_REGEX)

# Create executable
add_executable(
        streaming_gateway_crow src/main.cpp
        src/cli.cpp
        src/http.cpp
        src/logging.h
)

# Link Boost libraries
target_link_libraries(streaming_gateway_crow ${Boost_LIBRARIES})
