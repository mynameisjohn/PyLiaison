CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Project needs c++14 for std::integer_sequence
SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# Default build type
if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
  FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Use the CMake find python package
FIND_PACKAGE(PythonLibs)

# The FindPythonLibs module doesn't give us the lib directory, 
# so use the library path to find it with GET_FILENAME_COMPONENT
GET_FILENAME_COMPONENT(PYTHON_LIB_DIR ${PYTHON_LIBRARIES} DIRECTORY )

# Declare PyLiaison library
ADD_LIBRARY(PyLiaison
    ${CMAKE_CURRENT_SOURCE_DIR}/pyliaison.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pyliaison.h)

# We need the include path and the python header file location
TARGET_INCLUDE_DIRECTORIES(PyLiaison PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIRS})