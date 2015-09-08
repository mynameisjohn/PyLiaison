cmake_minimum_required(VERSION 2.8)

project(PyLiaison)

set(CMAKE_CXX_STANDARD 11)

set(PYTHON_LIBRARY C:/Python34/libs/python34_d.lib)
set(PYTHON_INCLUDE_DIR C:/Python34/include)

#find_package(PythonLibs 2.7 REQUIRED)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(PyLiaison ${SOURCES} ${HEADERS})
target_include_directories(PyLiaison PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PYTHON_INCLUDE_DIR})
#find_package(PythonLibs 2.7 REQUIRED)

add_executable(PyLiaison_Test main.cpp)
target_link_libraries(PyLiaison_Test LINK_PUBLIC PyLiaison ${PYTHON_LIBRARY})
