#pragma once

#include "core_types.h"
#include "point.h"
#include "containers/array.h"

/**
 * @group Utils Utility functions
 */
namespace Utils
{
	
	/// @brief Generates a random input file
	FORCE_INLINE void generateRandomInput(uint64 count = 1024 * 1024, const char * filename = "temp.txt")
	{
		// Open file
		FILE * fp = fopen(filename, "w");
		if (!fp) return;

		// Write random 2d points
		for (uint64 i = 0; i < count; ++i)
			fprintf(fp, "%f,%f\n", rand() / static_cast<float32>(RAND_MAX), rand() / static_cast<float32>(RAND_MAX));

		// Finalize
		fclose(fp);
	}

	/// @brief Parse input file
	FORCE_INLINE Array<point> parseInput(const char * filename = "temp.txt")
	{
		Array<point> out;

		// Open file
		FILE * fp = fopen(filename, "r");
		if (!fp) return out;

		// Read
		point p;
		char buffer[256];
		while (fscanf(fp, "%f,%f", p.data, p.data + 1) > 0)
			out.push_back(p);
		
		// Finalize
		fclose(fp);
		return out;
	}

	/// @brief Returns k furthest objects
	template<typename T>
	FORCE_INLINE Array<T> getKFurthest(const Array<T> & pool, uint64 k)
	{
		if (pool.empty())
		{
			throw std::invalid_argument("Empty pool");
		}
		// Out array
		Array<T> out;
		out.reserve(k);

		// Add first randomly
		const uint64 i = rand() % pool.size();
		out.push_back(pool[i]);

		while (out.size() < k)
		{

			float maxDist = 0.f;
			const T * furthest = nullptr;

			for (const T & elem : pool)
			{
				float dist = 0.f;
				uint8 discard = 0;

				for (const T & edge : out)
				{
					const float d = elem >> edge;
					discard = d == 0.f;

					dist += d;
				}

				if (dist > maxDist & !discard)
				{
					maxDist = dist;
					furthest = &elem;
				}
			}

			// Add furthest point
			out.push_back(*furthest);
		}

		return out;
	}
} // Utils