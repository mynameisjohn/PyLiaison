cmake_minimum_required(VERSION 2.8)

project(PyLiason)

set(CMAKE_CXX_STANDARD 11)

find_package(PythonLibs 2.7 REQUIRED)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(pyliason ${SOURCES} ${HEADERS})
target_include_directories(pyliason PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PYTHON_INCLUDE_DIRS})
find_package(PythonLibs 2.7 REQUIRED)

add_executable(PyLiason_Test main.cpp)
target_link_libraries(PyLiason_Test LINK_PUBLIC pyliason ${PYTHON_LIBRARIES})
