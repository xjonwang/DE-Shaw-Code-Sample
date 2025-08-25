cmake_minimum_required(VERSION 3.12)

# Download Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

set(TEST_SOURCES
    ShmTests.cpp
    ShmMessages.cpp
    MQMessages.cpp
)
list(TRANSFORM TEST_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

# Find Google Test
enable_testing()

add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE ${INCLUDE_DIRECTORIES})
target_link_libraries(tests gtest_main shm shm_message mq_message)
target_compile_definitions(tests PRIVATE UNIT_TEST)

# Add the tests
include(GoogleTest)
gtest_discover_tests(tests)
