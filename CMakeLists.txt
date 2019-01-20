cmake_minimum_required(VERSION 3.12)
project(mpi_project)

set(CMAKE_CXX_STANDARD 17)

include_directories(src/public src/public/data)

file(GLOB_RECURSE SOURCE_FILES
        private/*.c
        private/*.cc
        private/*.cpp
        private/*.cxx)

file(GLOB_RECURSE HEADER_FILES # CMake doesn't require header files, just to ease editor auto-completion

        public/*.h
        public/*.hpp
        public/*.tpp
        )


set(CMAKE_CXX_COMPILER /usr/local/bin/g++-6)

# openmp
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

# mpi
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/Cellar/open-mpi/3.1.2/include\
                                        -L/usr/local/opt/libevent/lib\
                                        -L/usr/local/Cellar/open-mpi/3.1.2/lib -lmpi")

add_executable(${PROJECT_NAME} main.cpp
        ${SOURCE_FILES}
        ${HEADER_FILES})