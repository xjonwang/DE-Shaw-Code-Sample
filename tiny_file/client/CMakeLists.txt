cmake_minimum_required(VERSION 3.12)
project(TinyFileClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(INCLUDE_DIRECTORIES
    include
    ../shared/include
    ../shared/include/mq_client_message
    ../shared/include/mq_server_message
    ../shared/include/shm_client_message
    ../shared/include/shm_server_message
    ../shared/include/shm
    ../service/include
    ../service/include/scheduling
)
list(TRANSFORM INCLUDE_DIRECTORIES PREPEND ${PROJECT_SOURCE_DIR}/)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/../shared/src/*.cpp ${PROJECT_SOURCE_DIR}/../service/src/*.cpp)

add_library(tiny_file_client ${SOURCES})
target_include_directories(tiny_file_client PRIVATE ${INCLUDE_DIRECTORIES})
target_link_libraries(tiny_file_client ${CMAKE_CURRENT_SOURCE_DIR}/lib/libsnappy.a)
target_compile_options(tiny_file_client PRIVATE -Wall -Wextra -Wpedantic -Werror)


add_subdirectory(app)
