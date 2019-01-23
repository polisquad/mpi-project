cmake_minimum_required(VERSION 3.12)

# Project setup ---------------------------------
project(kmeans)

## Platform setup
include(CMakePlatform.cmake)

## Versioning
set(SGL_VERSION_MAJOR 0)
set(SGL_VERSION_MINOR 0)
set(SGL_VERSION_PATCH 0) # Not really using it

## Build type config
set(CMAKE_CXX_FLAGS_DEBUG			"-DBUILD_DEBUG=1 -ggdb -O0")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO	"-DBUILD_DEVELOPMENT=1 -ggdb -O3")
set(CMAKE_CXX_FLAGS_RELEASE			"-DBUILD_RELEASE=1 -Ofast")

## Output config
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Compiler setup --------------------------------
## CXX preferences
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAG} -fopenmp -pthread")

# Code setup ------------------------------------
## Source code
add_subdirectory(src)

## Test setup -----------------------------------
option(SGL_UNIT_TEST "enable unit testing" OFF)
if(SGL_UNIT_TEST)
	add_subdirectory(test)
endif(SGL_UNIT_TEST)