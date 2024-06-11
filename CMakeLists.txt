cmake_minimum_required(VERSION 3.10.0)
project(chat LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(WARNINGS -Wall -Wpedantic -Wextra -Wno-missing-field-initializers -Werror)
set(SANITIZERS -fsanitize=address -fno-omit-frame-pointer)
#set(SANITIZERS -fsanitize=thread -fno-omit-frame-pointer)

add_subdirectory(docopt.cpp)
include_directories(docopt.cpp)

add_executable(chat chat.cpp)
target_link_libraries(chat docopt)
target_compile_options(chat PRIVATE "${WARNINGS}" "${SANITIZERS}")
target_link_options(chat PRIVATE "${SANITIZERS}")
