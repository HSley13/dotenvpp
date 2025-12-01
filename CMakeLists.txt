# Minimum CMake version required for C++20 support and modern features.
cmake_minimum_required(VERSION 3.21)

# Project metadata.
project(dotenvpp
    VERSION     0.1.0
    DESCRIPTION "A C++20 library that recursively searches for and loads .env files"
    LANGUAGES   CXX
)

# Prevent in-source builds to keep the source tree clean.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not allowed.\n"
        "Please create a separate build directory:\n"
        "  mkdir build && cd build && cmake .."
    )
endif()
