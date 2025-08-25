cmake_minimum_required(VERSION 3.12)
project(TinyFileService)

option(ENABLE_TESTS "Enable tests" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(INCLUDE_DIRECTORIES
    include
    include/scheduling
    ../shared/include
    ../shared/include/shm
    ../shared/include/mq_client_message
    ../shared/include/mq_server_message
    ../shared/include/shm_client_message
    ../shared/include/shm_server_message
)
list(TRANSFORM INCLUDE_DIRECTORIES PREPEND ${PROJECT_SOURCE_DIR}/)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/../shared/src/*.cpp)

add_executable(tiny_file_service ${SOURCES})
target_include_directories(tiny_file_service PRIVATE ${INCLUDE_DIRECTORIES})
target_link_libraries(tiny_file_service ${CMAKE_CURRENT_SOURCE_DIR}/lib/libsnappy.a)
target_compile_options(tiny_file_service PRIVATE -Wall -Wextra -Wpedantic -Werror)


if (ENABLE_TESTS)
    # Shm Library
    set(SHM_SOURCES
        ../shared/src/shm/ShmManager.cpp
        ../shared/src/shm/AbstractShmSegment.cpp
        ../shared/src/shm/ContiguousShmSegment.cpp
        ../shared/src/shm/FragmentedShmSegment.cpp
        ../shared/src/CommunicationManager.cpp
    )
    add_library(shm ${SHM_SOURCES})
    target_include_directories(shm PRIVATE ${INCLUDE_DIRECTORIES})

    # ShmMessage Library
    file (GLOB_RECURSE SHM_MESSAGE_SOURCES ${PROJECT_SOURCE_DIR}/../shared/src/shm_client_message/*.cpp ${PROJECT_SOURCE_DIR}/../shared/src/shm_server_message/*.cpp)
    add_library(shm_message ${SHM_MESSAGE_SOURCES})
    target_include_directories(shm_message PRIVATE ${INCLUDE_DIRECTORIES})

    # MQMessage Library
    file (GLOB_RECURSE MQ_MESSAGE_SOURCES ${PROJECT_SOURCE_DIR}/../shared/src/mq_client_message/*.cpp ${PROJECT_SOURCE_DIR}/../shared/src/mq_server_message/*.cpp ${PROJECT_SOURCE_DIR}/src/*.cpp)
    add_library(mq_message ${MQ_MESSAGE_SOURCES})
    target_link_libraries(mq_message ${PROJECT_SOURCE_DIR}/lib/libsnappy.a)
    target_include_directories(mq_message PRIVATE ${INCLUDE_DIRECTORIES})

    message(STATUS "Tests enabled")
    add_subdirectory(tests)
endif()
