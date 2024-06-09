cmake_minimum_required(VERSION 3.27.0)
project(chat VERSION 0.1.0 LANGUAGES C CXX)

add_executable(server server.cpp)
add_executable(client client.cpp)

