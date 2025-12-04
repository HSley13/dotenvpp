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

# Build options — enabled by default only when this is the top-level project.
option(DOTENVPP_BUILD_TESTS "Build the dotenvpp test suite" ${PROJECT_IS_TOP_LEVEL})
option(DOTENVPP_INSTALL     "Generate install targets"      ${PROJECT_IS_TOP_LEVEL})

# Define the library target and its ALIAS for consistent usage via find_package or add_subdirectory.
add_library(dotenvpp
    src/dotenv.cpp
    src/parser.cpp
    src/searcher.cpp
)
add_library(dotenvpp::dotenvpp ALIAS dotenvpp)

# Require C++20 (PUBLIC so consumers inherit it automatically).
target_compile_features(dotenvpp PUBLIC cxx_std_20)

# Expose the include/ directory for both build-tree and installed usage.
target_include_directories(dotenvpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Embed the project root as a compile-time fallback for cmake_source_dir().
target_compile_definitions(dotenvpp
    PRIVATE
        DOTENVPP_SOURCE_DIR_HINT=${CMAKE_SOURCE_DIR}
)

# Enable strict warnings only when building standalone (not as a subdependency).
if(PROJECT_IS_TOP_LEVEL)
    if(MSVC)
        target_compile_options(dotenvpp PRIVATE /W4 /WX)
    else()
        target_compile_options(dotenvpp PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -Wno-unused-parameter
        )
    endif()
endif()

# Optionally build and register the test suite with CTest.
if(DOTENVPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
