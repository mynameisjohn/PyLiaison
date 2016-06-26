cmake_minimum_required(VERSION 2.8)

# Project needs c++14 for std::integer_sequence 
project(PyLiaison)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# These paths work for me on windows and unix, but they may need changing
if (WIN32)
	set(PYTHON_LIBRARY C:/Python35/libs/python35_d.lib)
	set(PYTHON_INCLUDE_DIR C:/Python35/include)
else(WIN32)
	set(PYTHON_LIBRARY /usr/local/lib/libpython3.5m.a)
	set(PYTHON_INCLUDE_DIR /usr/local/include/python3.5m)
endif(WIN32)

# Several headers, just one source file
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

# Declare PyLiaison library
add_library(PyLiaison ${SOURCES} ${HEADERS})

# We need the include path and the python header file location
target_include_directories(PyLiaison PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PYTHON_INCLUDE_DIR})

# Add several tests