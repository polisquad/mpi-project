#pragma once

#include "core_types.h"

/**
 * @group Utils Utility functions
 */
namespace Utils
{
	
	/// @brief Generates a random input file
	void generateRandomInput(const char * filename = "temp/input.txt")
	{
		// Open file
		FILE * fp = fopen(filename, "w");
		if (!fp) return;

		// Write random 2d points
		for (uint64 i = 0; i < 1024 * 1024 * 2; ++i)
			fprintf(fp, "%f,%f\n", rand() / static_cast<float32>(RAND_MAX), rand() / static_cast<float32>(RAND_MAX));

		// Finalize
		fclose(fp);
	}
} // Utils