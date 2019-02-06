cmake_minimum_required(VERSION 3.12)

# Project setup ---------------------------------
project(kmeans)


file(GLOB_RECURSE SOURCE_FILES
        private/*.c
        private/*.cc
        private/*.cpp
		private/*.cxx
)

file(GLOB_RECURSE HEADER_FILES # CMake doesn't require header files, just to ease editor auto-completion

        public/*.h
        public/*.hpp
        public/*.tpp
)

# Basic platform setup --------------------------------
if(UNIX)
	add_compile_definitions(PLATFORM_UNIX=1)
elseif(WIN32)
	add_compile_definitions(PLATFORM_WINDOWS=1)
elseif(APPLE)
	add_compile_definitions(PLATFORM_APPLE=1)
endif(UNIX)

# Compiler setup --------------------------------
## CXX preferences
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAG} -mavx2 -fopenmp -pthread")

# Code setup ------------------------------------
## Source code
add_subdirectory(src)

## Test setup -----------------------------------
option(SGL_UNIT_TEST "enable unit testing" OFF)
if(SGL_UNIT_TEST)
	add_subdirectory(test)
endif(SGL_UNIT_TEST)
