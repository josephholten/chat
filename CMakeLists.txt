cmake_minimum_required(VERSION 3.10.0)
project(chat LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(WARNINGS -Wall -Wpedantic -Wextra -Wno-missing-field-initializers -Werror)
set(SANITIZERS -fsanitize=address -fno-omit-frame-pointer)

add_subdirectory(docopt)
include_directories(docopt)

add_subdirectory(spdlog)

add_subdirectory(chat)
