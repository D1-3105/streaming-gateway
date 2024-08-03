cmake_minimum_required(VERSION 3.10)
project(FloWWeaver)

# Compiler options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc")

# Boost+Crow
include_directories(${Boost_INCLUDE_DIRS})
set(CROW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/crow")
include_directories(${CROW_INCLUDE_DIR})

# Boost.ASIO
add_definitions(-DCROW_USE_BOOST)
add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_BOOST_DATE_TIME_NO_LIB)
add_definitions(-DASIO_DISABLE_BOOST_ARRAY)
add_definitions(-DASIO_DISABLE_BOOST_BIND)
add_definitions(-DASIO_DISABLE_BOOST_DATE_TIME)
add_definitions(-DASIO_DISABLE_BOOST_REGEX)

add_subdirectory(common)

add_executable(
        flowweave

        src/main.cpp
        src/http.cpp
)

target_link_libraries(flowweave shared_memory_manager common)

target_compile_options(flowweave PUBLIC -g)
target_link_options(flowweave PUBLIC -g)
target_compile_options(shared_memory_manager PUBLIC -g)
target_link_options(shared_memory_manager PUBLIC -g)
