CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Make sure we have some python exe as hint
# if was already specified as absolute then leave it alone
if (NOT(IS_ABSOLUTE "${PYTHON_EXECUTABLE}"))
	FIND_PROGRAM(PYTHON_EXECUTABLE python)
endif (NOT(IS_ABSOLUTE "${PYTHON_EXECUTABLE}"))

# Project needs c++14 for std::integer_sequence
SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# Default build type
IF(NOT CMAKE_BUILD_TYPE)
SET(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
  FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Use the CMake find python package
FIND_PACKAGE(PythonLibs REQUIRED)

# Declare PyLiaison library
ADD_LIBRARY(PyLiaison
    ${CMAKE_CURRENT_SOURCE_DIR}/pyliaison.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pyliaison.h)

# The FindPythonLibs module doesn't give us the lib directory, 
# so use the library path to find it with GET_FILENAME_COMPONENT
#GET_FILENAME_COMPONENT(PYTHON_LIB_DIR "${PYTHON_LIBRARIES}" DIRECTORY )

# Adding PyLiaison as a target gives us the pyl and Python include paths
TARGET_INCLUDE_DIRECTORIES(PyLiaison PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIRS})

TARGET_LINK_LIBRARIES(PyLiaison PUBLIC ${PYTHON_LIBRARIES})

