cmake_minimum_required(VERSION 3.27.0)
project(chat LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(WARNINGS -Wall -Wpedantic -Wextra -Wno-missing-field-initializers)

add_subdirectory(docopt.cpp)
include_directories(docopt.cpp)

add_executable(server server.cpp)
target_link_libraries(server docopt)
target_compile_options(server PUBLIC "${WARNINGS}")

add_executable(client client.cpp)
target_compile_options(client PUBLIC "${WARNINGS}")
