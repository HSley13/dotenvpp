# Build the test executable and link against the library.
add_executable(dotenvpp_tests test_dotenv.cpp)
target_link_libraries(dotenvpp_tests PRIVATE dotenvpp)

# Register with CTest and pass the source dir so Searcher::cmake_source_dir() works.
add_test(NAME dotenvpp_tests COMMAND dotenvpp_tests)
set_tests_properties(dotenvpp_tests PROPERTIES
    ENVIRONMENT "DOTENVPP_CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
)
