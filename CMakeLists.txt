CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(pylTests)

# Use the CMake find python package
SET(Python_ADDITIONAL_VERSIONS 3.4)
FIND_PACKAGE(PythonLibs)

# We use c++14 for std::integer_sequence
SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# Add subdir for Pyliaison
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pyl)

# Set up include / lib directories
# The FindPythonLibs module doesn't give us the lib directory, 
# so use the library path to find it with GET_FILENAME_COMPONENT
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIRS})
LINK_DIRECTORIES(${PYTHON_LIB_DIR})

# On Windows it seems like the python headers link the
# correct library automatically, but on other platforms
# make sure to link against the python libraries
IF(WIN32)
	SET(PYL_LIB PyLiaison)
ELSE(WIN32)
	SET(PYL_LIB PyLiaison ${PYTHON_LIBRARIES})
ENDIF(WIN32)

# Add test executables

# Hello World
ADD_EXECUTABLE(pylHelloWorld ${CMAKE_CURRENT_SOURCE_DIR}/test/pylHelloWorld.cpp)
TARGET_LINK_LIBRARIES(pylHelloWorld LINK_PUBLIC ${PYL_LIB} )

# Test Script
ADD_EXECUTABLE(pylTestScript ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestScript.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestScript.py)
TARGET_LINK_LIBRARIES(pylTestScript LINK_PUBLIC ${PYL_LIB} )

# Test Modules
ADD_EXECUTABLE(pylTestModules ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestModules.cpp)
TARGET_LINK_LIBRARIES(pylTestModules LINK_PUBLIC ${PYL_LIB} )

# Test Classes
ADD_EXECUTABLE(pylTestClasses ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestClasses.cpp)
TARGET_LINK_LIBRARIES(pylTestClasses LINK_PUBLIC ${PYL_LIB} )

# Test overloads
ADD_EXECUTABLE(pylTestOverloads ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestOverloads.cpp)
TARGET_LINK_LIBRARIES(pylTestOverloads LINK_PUBLIC ${PYL_LIB} )
