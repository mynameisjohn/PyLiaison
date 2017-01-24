CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(pylTests)

# Add subdir for Pyliaison
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pyl)

# Set up include / lib directories
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_INCLUDE_DIR})
LINK_DIRECTORIES(${PYTHON_LIB_DIR})

# Add test executables

# Hello world
ADD_EXECUTABLE(pylHelloWorld ${CMAKE_CURRENT_SOURCE_DIR}/test/pylHelloWorld.cpp)
TARGET_LINK_LIBRARIES(pylHelloWorld LINK_PUBLIC PyLiaison)
