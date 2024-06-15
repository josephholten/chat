include_directories(..)

add_executable(chatd chatd.cpp)
target_link_libraries(chatd docopt spdlog::spdlog)
target_compile_options(chatd PRIVATE "$<$<CONFIG:DEBUG>:${WARNINGS}>" "$<$<CONFIG:DEBUG>:${SANITIZERS}>")
target_link_options(chatd PRIVATE "$<$<CONFIG:DEBUG>:${SANITIZERS}>")

add_executable(chat chat.cpp)
target_link_libraries(chat docopt spdlog::spdlog)
target_compile_options(chat PRIVATE "$<$<CONFIG:DEBUG>:${WARNINGS}>" "$<$<CONFIG:DEBUG>:${SANITIZERS}>")
target_link_options(chat PRIVATE "$<$<CONFIG:DEBUG>:${SANITIZERS}>")