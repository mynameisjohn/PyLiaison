cmake_minimum_required(VERSION 2.8)

project(PyLiaison)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

if (WIN32)
	set(PYTHON_LIBRARY C:/Python35/libs/python35_d.lib)
	set(PYTHON_INCLUDE_DIR C:/Python35/include)
	add_definitions(-DGLEW_STATIC)
else(WIN32)
	set(PYTHON_LIBRARY /usr/local/lib/libpython3.5m.a)
	set(PYTHON_INCLUDE_DIR /usr/local/include/python3.5m)
endif(WIN32)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(PyLiaison ${SOURCES} ${HEADERS})
target_include_directories(PyLiaison PUBLIC /usr/local/include C:/Libraries/include ${CMAKE_CURRENT_SOURCE_DIR}/include ${PYTHON_INCLUDE_DIR})

add_executable(PyLiaison_Test test/main.cpp test/pylTestClasses.h)
target_include_directories(PyLiaison_Test ${CMAKE_CURRENT_SOURCE_DIR}/test)
target_link_libraries(PyLiaison_Test LINK_PUBLIC PyLiaison ${PYTHON_LIBRARY})
