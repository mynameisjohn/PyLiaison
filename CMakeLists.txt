cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

find_package(PythonLibs 2.7 REQUIRED)

add_library(pyliason pyliason.cpp pyliason.h)
target_include_directories(pyliason PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIRS})