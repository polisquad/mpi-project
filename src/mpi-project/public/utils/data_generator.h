#pragma once

#include "core_types.h"
#include "containers/point.h"

#include <omp.h>
#include <vector>

/**
 * @class DataGenerator utils/data_generator.h
 * 
 * A very simple data generator
 */
template<typename T>
class DataGenerator
{
protected:
	/// Num of data points to generate
	uint64 numDataPoints;

	/// Num of clusters
	uint32 numClusters;

	/// Data point dimension
	uint32 dataDim;

public:
	/// Default constructor
	FORCE_INLINE DataGenerator(uint64 _numDataPoints, uint32 _numClusters = 5, uint32 _dataDim = 2) :
		numDataPoints(_numDataPoints),
		numClusters(_numClusters),
		dataDim(_dataDim) {}
	
	/// Generate new dataset
	std::vector<Point<T>> generate()
	{
		// In order to generate a dataset
		// suitable for kmeans, we compute
		// k points in space and randomly
		// draw inside circles centered there
		
		// Num points in cluster
		const uint32 clusterLoad = numDataPoints / numClusters;

		// Dataset
		std::vector<Point<T>> out(clusterLoad * numClusters);

		for (uint32 k = 0; k < numClusters; ++k)
		{
			const uint64 offset = k * (uint64)clusterLoad;

			// Cluster points
			std::vector<Point<T>> clusterPoints(clusterLoad);

			// Get random point
			Point<T> center(dataDim);
			for (uint32 j = 0; j < dataDim; ++j) center[j] = rand() / (float32)RAND_MAX;

			// Get random cluster radius
			float32 clusterRadius = rand() / ((float32)RAND_MAX * 5.f) + 0.1f;

			// Draw around
			#pragma omp parallel
			{
				// Reseed random
				srand(clock() ^ omp_get_thread_num());

				#pragma omp for nowait
				for (uint32 i = 0; i < clusterLoad; ++i)
				{
					// Get random point
					Point<T> p(dataDim);
					for (uint32 j = 0; j < dataDim; ++j) p[j] = rand() / (float32)RAND_MAX;

					// Clamp this inside cluster radius
					Point<T> r = p - center;
					const float32 dist = r.getSize();
					
					// Randomly distribute in circle
					p = center + r * (clusterRadius / dist) * (rand() / (float32)RAND_MAX);

					out[offset + i] = p;
				}
			}
		}

		return out;
	}
};