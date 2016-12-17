cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
project (server)

set(SOURCE_EXE Hello_world.cpp)
set(SOURCE_LIB easy.cpp)

add_library(lib STATIC ${SOURCE_LIB})
add_executable(server ${SOURCE_EXE})

target_link_libraries(server lib)
