cmake_minimum_required(VERSION 2.8)

project(PyLiason)

set(CMAKE_CXX_STANDARD 11)

set(PYTHON_LIBRARY C:/Python34/libs/python34_d.lib)
set(PYTHON_INCLUDE_DIR C:/Python34/include)

#find_package(PythonLibs 2.7 REQUIRED)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(pyliason ${SOURCES} ${HEADERS})
target_include_directories(pyliason PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PYTHON_INCLUDE_DIR})
#find_package(PythonLibs 2.7 REQUIRED)

add_executable(PyLiason_Test main.cpp)
target_link_libraries(PyLiason_Test LINK_PUBLIC pyliason ${PYTHON_LIBRARY})
