cmake_minimum_required(VERSION 3.27.0)
project(chat LANGUAGES C CXX)

set(WARNINGS -Wall -Wpedantic -Wextra)

add_subdirectory(docopt.cpp)

add_executable(chat chat.cpp)
target_link_libraries(chat docopt.cpp)
target_compile_options(chat PUBLIC "${WARNINGS}")
