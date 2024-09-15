cmake_minimum_required(VERSION 3.30.1)

project(EDFS)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(server server.cpp)
target_link_libraries(server PRIVATE ws2_32 ssl crypto)

add_executable(client client.cpp)
target_link_libraries(client PRIVATE ws2_32 ssl crypto)