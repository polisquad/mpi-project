# Setup executable ------------------------------
## Find source files
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

## Define executable
add_executable(${PROJECT_NAME}

        ./main.cpp
        ${SOURCE_FILES}
        ${HEADER_FILES}
        private/data.cpp public/data.h private/csv.cpp public/csv.h)

## Load libraries
find_package(MPI REQUIRED)
if (NOT MPI_FOUND OR NOT MPI_CXX_FOUND)
    message(FATAL_ERROR "MPI not found or misconfigured")
endif ()

## Link configuration
target_link_libraries(${PROJECT_NAME}

        ${MPI_C_LIBRARIES}
        ${MPI_CXX_LIBRARIES}
        )

## Include directories
target_include_directories(${PROJECT_NAME}

        PUBLIC
        public
        ${MPI_C_INCLUDE_DIRS}
        ${MPI_CXX_INCLUDE_DIRS}
        )

## Add run target
add_custom_target(run

        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        )