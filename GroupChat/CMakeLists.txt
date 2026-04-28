cmake_minimum_required(VERSION 3.10)
project(GroupChat)

set(CMAKE_CXX_STANDARD 17)

add_executable(server server/main.cpp)
add_executable(client client/main.cpp)

target_link_libraries(server pthread)
target_link_libraries(client pthread)
