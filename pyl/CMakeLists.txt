CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Project needs c++14 for std::integer_sequence
SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# Specify the python version
# Be wary of doing something like
# 3.5 instead of 35, etc.
MESSAGE("\n")
IF(NOT(${_PY_VER}))
    SET(_PY_VER "35")
    ADD_DEFINITIONS("-D_PY_VER=${_PY_VER}")
    MESSAGE("PyLiaison: No Python version specified! Defaulting to ${_PY_VER}")
ELSE(NOT(${_PY_VER}))
    MESSAGE("PyLiaison: Python version specified as ${_PY_VER}")
ENDIF(NOT(${_PY_VER}))

# Set library directory
# uses version specified and some familiar paths,
# but can be specified on the command line
IF(NOT(${_PYTHON_LIB_DIR}))
    IF (WIN32)
        SET(_PYTHON_LIB_DIR "C:/Python${_PY_VER}/libs")
    ELSEIF(APPLE)
        SET(_PYTHON_LIB_DIR "/usr/local/lib")
    ELSEIF(UNIX)
        SET(_PYTHON_LIB_DIR "/usr/lib/x86_64-linux-gnu")
    ENDIF(WIN32)
    MESSAGE("PyLiaison: No Python library dir specified! Defaulting to ${_PYTHON_LIB_DIR}")
ELSE(NOT(${_PYTHON_LIB_DIR}))
    MESSAGE("PyLiaison: Python library dir set to ${_PYTHON_LIB_DIR}...")
ENDIF(NOT(${_PYTHON_LIB_DIR}))

# Set include directory
# uses version specified and some familiar paths,
# but can be specified on the command line
IF(NOT(${_PYTHON_INCLUDE_DIR}))
    IF (WIN32)
        SET(_PYTHON_INCLUDE_DIR "C:/Python${_PY_VER}/include")
    ELSEIF(APPLE)
        SET(_PYTHON_INCLUDE_DIR "/usr/local/include/python${_PY_VER}m")
    ELSEIF(UNIX)
        SET(_PYTHON_INCLUDE_DIR "/usr/include/python${_PY_VER}m")
    ENDIF(WIN32)
    MESSAGE("PyLiaison: No Python include dir specified! Defaulting to ${_PYTHON_INCLUDE_DIR}")
ELSE(NOT(${_PYTHON_INCLUDE_DIR}))
    MESSAGE("PyLiaison: Python include dir set to ${_PYTHON_INCLUDE_DIR}...")
ENDIF(NOT(${_PYTHON_INCLUDE_DIR}))
MESSAGE("\n")

# Move variables up to parent scope
SET(PYTHON_LIB_DIR ${_PYTHON_LIB_DIR} PARENT_SCOPE)
SET(PYTHON_INCLUDE_DIR ${_PYTHON_INCLUDE_DIR} PARENT_SCOPE)

# Declare PyLiaison library
ADD_LIBRARY(PyLiaison 
    ${CMAKE_CURRENT_SOURCE_DIR}/pyliaison.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/pyliaison.h)

# We need the include path and the python header file location
TARGET_INCLUDE_DIRECTORIES(PyLiaison PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${_PYTHON_INCLUDE_DIR})
