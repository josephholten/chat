cmake_minimum_required(VERSION 3.27.0)
project(chat LANGUAGES C CXX)

set(WARNINGS -Wall -Wpedantic -Wextra)

add_subdirectory(docopt.cpp)

add_executable(server server.cpp)
target_link_libraries(server docopt.cpp)
target_compile_options(server PUBLIC "${WARNINGS}")

add_executable(client client.cpp)
target_compile_options(client PUBLIC "${WARNINGS}")
