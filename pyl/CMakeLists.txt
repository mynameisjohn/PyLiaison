CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Project needs c++14 for std::integer_sequence
SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# These paths work for me on windows and unix, but they may need changing
IF (WIN32)
	# This works for me on windows for python3.5
	# (though I occasionally link debug)
  SET(PYTHON_LIB_DIR C:/Python35/libs)
	SET(PYTHON_LIBRARY python35.lib)
	SET(PYTHON_INCLUDE_DIR C:/Python35/include)
ELSEIF(APPLE)
	# This is where brew put the python dev libraries
  SET(PYTHON_LIB_DIR /usr/local/lib)
	SET(PYTHON_LIBRARY python3.5m.a)
	SET(PYTHON_INCLUDE_DIR /usr/local/include/python3.5m)
ELSEIF(UNIX)
  # This is where I found the python3 libraries on ubuntu	(YMMV)
  SET(PYTHON_LIB_DIR /usr/lib/x86_64-linux-gnu)
	SET(PYTHON_LIBRARY python3.5m.so)
	SET(PYTHON_INCLUDE_DIR /usr/include/python3.5m)
ENDIF(WIN32)

# Several headers, just one source file
SET(PYL_INC ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIR})
FILE(GLOB PYL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*)

# Move variables up to parent scope
SET(PYTHON_LIB_DIR ${PYTHON_LIB_DIR} PARENT_SCOPE)
SET(PYTHON_LIBRARY ${PYTHON_LIBRARY} PARENT_SCOPE)
SET(PYTHON_INCLUDE_DIR ${PYTHON_INCLUDE_DIR} PARENT_SCOPE)
SET(PYL_INC ${PYL_INC} PARENT_SCOPE)
SET(PYL_SRC ${PYL_SRC} PARENT_SCOPE)

# Declare PyLiaison library
ADD_LIBRARY(PyLiaison ${PYL_SRC})

# We need the include path and the python header file location
TARGET_INCLUDE_DIRECTORIES(PyLiaison PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIR})
