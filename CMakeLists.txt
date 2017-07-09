CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(pylTests)

SET(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# Add subdir for Pyliaison
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pyl)

# Set up include / lib directories
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIR})
LINK_DIRECTORIES(${PYTHON_LIB_DIR})

# Add test executables

# Hello World
ADD_EXECUTABLE(pylHelloWorld ${CMAKE_CURRENT_SOURCE_DIR}/test/pylHelloWorld.cpp)
TARGET_LINK_LIBRARIES(pylHelloWorld LINK_PUBLIC PyLiaison ${PYTHON_LIB})

# Test Script
ADD_EXECUTABLE(pylTestScript ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestScript.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestScript.py)
TARGET_LINK_LIBRARIES(pylTestScript LINK_PUBLIC PyLiaison ${PYTHON_LIB})

# Test Modules
ADD_EXECUTABLE(pylTestModules ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestModules.cpp)
TARGET_LINK_LIBRARIES(pylTestModules LINK_PUBLIC PyLiaison ${PYTHON_LIB})

# Test Classes
ADD_EXECUTABLE(pylTestClasses ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestClasses.cpp)
TARGET_LINK_LIBRARIES(pylTestClasses LINK_PUBLIC PyLiaison ${PYTHON_LIB})

# Test overloads
ADD_EXECUTABLE(pylTestOverloads ${CMAKE_CURRENT_SOURCE_DIR}/test/pylTestOverloads.cpp)
TARGET_LINK_LIBRARIES(pylTestOverloads LINK_PUBLIC PyLiaison ${PYTHON_LIB})