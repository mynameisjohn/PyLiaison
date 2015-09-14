cmake_minimum_required(VERSION 2.8)

project(PyLiaison)

if (WIN32)
	set(PYTHON_LIBRARY C:/Python34/libs/python34_d.lib)
	set(PYTHON_INCLUDE_DIR C:/Python34/include)
elseif(UNIX)
	set(PYTHON_LIBRARY /usr/lib/python3.4/lib/libpython3.4m.so)
	set(PYTHON_INCLUDE_DIR /usr/include/python3.4)
elseif(APPLE)
	set(PYTHON_LIBRARY /usr/local/Frameworks/Python.framework/Versions/3.4/lib/libpython3.4m.dylib)
   set(PYTHON_INCLUDE_DIR /usr/local/Frameworks/Python.framework/Versions/3.4/include/python3.4m/)
endif(WIN32)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(PyLiaison ${SOURCES} ${HEADERS})
target_include_directories(PyLiaison PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PYTHON_INCLUDE_DIR})

add_executable(PyLiaison_Test main.cpp)
target_link_libraries(PyLiaison_Test LINK_PUBLIC PyLiaison ${PYTHON_LIBRARY})
