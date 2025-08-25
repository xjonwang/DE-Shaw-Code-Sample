cmake_minimum_required(VERSION 3.12)

set(APP_SOURCES
    App.cpp
)
list(TRANSFORM APP_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

add_executable(app ${APP_SOURCES})
target_include_directories(app PRIVATE ${INCLUDE_DIRECTORIES})
target_link_libraries(app tiny_file_client)
target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic -Werror)
