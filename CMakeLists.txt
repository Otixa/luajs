cmake_minimum_required(VERSION 3.7)
project(luajs)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
    src/luajs.cc src/luastate.cpp src/luastate.h src/luajs_utils.cpp src/luajs_utils.h)

include_directories(/usr/local/include/node/)

add_executable(luajs ${SOURCE_FILES})