CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(pylTests)

# Use the CMake find python package
FIND_PACKAGE(PythonLibs)

# We use c++14 for std::integer_sequence
SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# Add subdir for Pyliaison
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pyl)

# Set up include / lib directories
# The FindPythonLibs module doesn't give us the lib directory, 
# so use the library path to find it with GET_FILENAME_COMPONENT
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIRS})
GET_FILENAME_COMPONENT(PYTHON_LIB_DIR ${PYTHON_LIBRARIES} DIRECTORY )
LINK_DIRECTORIES(${PYTHON_LIB_DIR})
SET(PYL_LIBS PyLiaison)

# Add test executables

# Hello World
ADD_EXECUTABLE(pylHelloWorld ${CMAKE_CURRENT_SOURCE_DIR}/test/pylHelloWorld.cpp)
TARGET_LINK_LIBRARIES(pylHelloWorld LINK_PUBLIC ${PYL_LIBS} )

# Test Script
ADD_EXECUTABLE(pylTestScript ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestScript.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestScript.py)
TARGET_LINK_LIBRARIES(pylTestScript LINK_PUBLIC ${PYL_LIBS} )

# Test Modules
ADD_EXECUTABLE(pylTestModules ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestModules.cpp)
TARGET_LINK_LIBRARIES(pylTestModules LINK_PUBLIC ${PYL_LIBS} )

# Test Classes
ADD_EXECUTABLE(pylTestClasses ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestClasses.cpp)
TARGET_LINK_LIBRARIES(pylTestClasses LINK_PUBLIC ${PYL_LIBS} )

# Test overloads
ADD_EXECUTABLE(pylTestOverloads ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestOverloads.cpp)
TARGET_LINK_LIBRARIES(pylTestOverloads LINK_PUBLIC ${PYL_LIBS} )