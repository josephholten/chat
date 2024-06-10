cmake_minimum_required(VERSION 3.27.0)
project(chat LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(WARNINGS -Wall -Wpedantic -Wextra -Wno-missing-field-initializers)

add_subdirectory(docopt.cpp)
include_directories(docopt.cpp)

add_executable(chat chat.cpp)
target_link_libraries(chat docopt)
target_compile_options(chat PUBLIC "${WARNINGS}")
