CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(pylTests)

# Use the CMake find python package
FIND_PACKAGE(PythonLibs REQUIRED)

# We use c++14 for std::integer_sequence
SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# Add subdir for Pyliaison
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/pyl)

# Add python link directory (only Windows needs this)
LINK_DIRECTORIES(${PYTHON_LIB_DIR})

# Add test executables

# Hello World
ADD_EXECUTABLE(pylHelloWorld ${CMAKE_CURRENT_SOURCE_DIR}/test/pylHelloWorld.cpp)
TARGET_LINK_LIBRARIES(pylHelloWorld LINK_PUBLIC PyLiaison )

# Test Script
ADD_EXECUTABLE(pylTestScript ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestScript.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestScript.py)
TARGET_LINK_LIBRARIES(pylTestScript LINK_PUBLIC PyLiaison )

# Test Modules
ADD_EXECUTABLE(pylTestModules ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestModules.cpp)
TARGET_LINK_LIBRARIES(pylTestModules LINK_PUBLIC PyLiaison )

# Test Classes
ADD_EXECUTABLE(pylTestClasses ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestClasses.cpp)
TARGET_LINK_LIBRARIES(pylTestClasses LINK_PUBLIC PyLiaison )

# Test overloads
ADD_EXECUTABLE(pylTestOverloads ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestOverloads.cpp)
TARGET_LINK_LIBRARIES(pylTestOverloads LINK_PUBLIC PyLiaison )
