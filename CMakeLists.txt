set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc")

set(CMAKE_CXX_STANDARD 20)

# Find Boost with required components
find_package(Boost REQUIRED COMPONENTS system thread log)
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

# Include directories for Boost and Crow
include_directories(${Boost_INCLUDE_DIRS})
set(CROW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/crow")
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
        src/shared_memory_manager.cpp
        src/shared_memory_manager.h
)

add_library(shared_memory_manager SHARED
        src/shared_memory_manager.cpp
        src/shared_memory_manager.h
)

add_executable(
        tests qa/test_shared_memory_manager.cpp
        src/logging.h
        src/shared_memory_manager.cpp
)

# Set debug flags for the executable and shared library
target_compile_options(streaming_gateway_crow PUBLIC -g)
target_link_options(streaming_gateway_crow PUBLIC -g)
target_compile_options(shared_memory_manager PUBLIC -g)
target_link_options(shared_memory_manager PUBLIC -g)
target_link_options(tests PUBLIC -g)

# Link Boost libraries
target_link_libraries(streaming_gateway_crow ${Boost_LIBRARIES} -lrt)
target_link_libraries(tests ${Boost_LIBRARIES} -lrt)
target_link_libraries(shared_memory_manager ${Boost_LIBRARIES} Boost::log)
