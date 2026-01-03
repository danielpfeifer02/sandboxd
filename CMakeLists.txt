cmake_minimum_required(VERSION 3.15)
project(sandboxd VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Build type defaults to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
)

# Source directories
add_subdirectory(sandbox)
add_subdirectory(cli)
# add_subdirectory(daemon)  # Will be added in later weeks
# add_subdirectory(tests)   # Will be added when tests are implemented

