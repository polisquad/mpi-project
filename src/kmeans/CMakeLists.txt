# Setup library ---------------------------------
## Gather source and headers files
file(GLOB_RECURSE SOURCES

	private/*.c
	private/*.cpp
	private/*.cxx
)
file(GLOB_RECURSE HEADERS public

	public/*.h
	public/*.hpp
	public/*.tpp
)

## Create library
add_library(${PROJECT_NAME}

	${SOURCES}
	${HEADERS}
)

## Manage dependencies
find_package(MPI REQUIRED)
link_libraries(${PROJECT_NAME}

	${MPI_C_LIBRARIES}
)
target_include_directories(${PROJECT_NAME}

	PUBLIC
		public
		${MPI_C_INCLUDE_DIRS}
)

# Setup executable -------------------------
add_executable(main main.cpp)

# Set output directory
set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/bin)
set_target_properties(main

	PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

# Link library
target_link_libraries(main ${PROJECT_NAME})

# Enable run target
add_custom_target(run

	COMMAND				${OUTPUT_DIR}/main
	DEPENDS				main
	WORKING_DIRECTORY	${PROJECT_SOURCE_DIR}
)