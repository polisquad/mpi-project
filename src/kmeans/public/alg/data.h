#pragma once

#include "coremin.h"
#include "parallel/mpi_globals.h"
#include "parallel/omp.h"
#include "parallel/threading.h"

#include <vector>

/**
 * Cluster for k-means algorithm.
 * 
 * T is the data type. T must implement
 * basic arithmetic methods (operator+,
 * operator*) and a @c getDistance()
 * method that returns the distance
 * between two data points. T should
 * be default constructible
 */
template<typename T>
class GCC_ALIGN(32) Cluster
{
public:
	/// MPI data type
	static MPI_Datatype type;

protected:
	/// Current centroid
	T centroid;

	/// Working centroid
	T workingCentroid;

	/// Working weight
	float32 workingWeight;

public:
	/// Default constructor
	FORCE_INLINE Cluster(typename ConstRef<T>::Type _centroid = T()) :
		centroid(_centroid),
		workingCentroid(),
		workingWeight(0.f) {}

	/// Reset current and working centroid
	FORCE_INLINE void reset(typename ConstRef<T>::Type data)
	{
		// Reset all
		centroid = data;
		workingCentroid = T();
		workingWeight = 0.f;
	}

	/// Get distance between two clusters (distance between centroids)
	FORCE_INLINE float32 getDistance(const Cluster<T> & cluster) const
	{
		return centroid.getDistance(cluster.centroid);
	}

	/// Get distance from a point
	FORCE_INLINE float32 getDistance(typename ConstRef<T>::Type data) const
	{
		return centroid.getDistance(data);
	}

	/// Add weight
	FORCE_INLINE void addWeight(typename ConstRef<T>::Type data, float32 weight = 1.f)
	{
		workingCentroid += data, workingWeight += weight;
	}

	/// Fuse with other cluster
	/// @{
	FORCE_INLINE const Cluster<T> & fuse(const Cluster<T> & cluster)
	{
		// Lock this mutex
		centroid = (centroid + cluster.centroid) * 0.5f;

		workingCentroid	+= cluster.workingCentroid;
		workingWeight	+= cluster.workingWeight;

		return *this;
	}
	static FORCE_INLINE const Cluster<T> fuse(const Cluster<T> & a, const Cluster<T> & b)
	{
		Cluster<T> c;

		c.centroid = (a.centroid + b.centroid) * 0.5f;
		c.workingCentroid	= a.workingCentroid + b.workingCentroid;
		c.workingWeight		= a.workingWeight + b.workingWeight;

		return c;
	}
	/// @}

	/// Commit weights
	FORCE_INLINE void commit()
	{
		// Update current centroid
		if (workingWeight)
			centroid = workingCentroid * (1.f / workingWeight);

		// Reset working set
		workingCentroid = T();
		workingWeight = 0.f;
	}

	/// Cluster initialization algorithms
	/// @{
	/// Randomly pick k centroids
	static std::vector<Cluster<T>> initRandom(const std::vector<T> & dataPoints, uint32 numClusters)
	{
		std::vector<Cluster<T>> clusters; clusters.reserve(numClusters);
		const uint32 numDataPoints = dataPoints.size();

		// Check that num of data points is sufficient
		if (numDataPoints <= numClusters)
		{
			for (const auto & dataPoint : dataPoints)
			{
				Cluster<T> cluster;
				cluster.reset(dataPoint);
				clusters.push_back(cluster);
			}
			
			return clusters;
		}

		// Randomly pick
		uint32 * pick = new uint32[numClusters];
		for (uint32 i = 0; i < numClusters; ++i)
		{
			uint32 idx;
			bool bDuplicate = false;

			// Pick random until no duplicate
			do
			{
				bDuplicate = false;
				idx = rand() % numDataPoints;

				for (uint32 k = 0; k < i & !bDuplicate; ++k)
					if (pick[k] == idx) bDuplicate = true;
			} while (bDuplicate);

			pick[i] = idx;
			
			Cluster<T> cluster(dataPoints[idx]);
			clusters.push_back(cluster);
		}

		return clusters;
	}

	/// Find k furthest centroids
	static std::vector<Cluster<T>> initFurthest(const std::vector<T> & dataPoints, uint32 numClusters)
	{
		std::vector<Cluster<T>> clusters; clusters.reserve(numClusters);
		const uint32 numDataPoints = dataPoints.size();

		if (numDataPoints <= numClusters)
		{
			for (const auto & dataPoint : dataPoints)
			{
				Cluster<T> cluster(dataPoint);
				clusters.push_back(cluster);
			}
			
			return clusters;
		}

	#if 1
		// Randomly choose first
		{
			Cluster<T> cluster;
			cluster.reset(dataPoints[rand() % numDataPoints]);
			clusters.push_back(cluster);
		}
	#elif 0
		{
			Cluster<T> cluster;

			// Choose smallest
			const T origin = T();
			uint32 smallest = 0;
			float32 minDist = dataPoints[0].getDistance(origin);

			for (uint32 i = 1; i < numDataPoints; ++i)
			{
				const auto & dataPoint = dataPoints[i];

				const float32 dist = dataPoint.getDistance(origin);
				if (dist < minDist)
					minDist = dist, smallest = i;
			}

			cluster.reset(dataPoints[smallest]);
			clusters.push_back(cluster);
		}	
	#else
		{
			Cluster<T> cluster;

			// Choose average
			T average = dataPoints[0];
			uint32 i = 1;
			for (; i < numDataPoints; ++i)
				average += dataPoints[i];

			cluster.reset(average * (1.f / i));
			clusters.push_back(cluster);
		}

	#endif

		while (clusters.size() < numClusters)
		{
			uint32 furthest = -1;
			float32 maxDist = 0.f;

			for (uint32 i = 1; i < numDataPoints; ++i)
			{
				const auto & dataPoint = dataPoints[i];
				float32 dist = FLT_MAX;

				for (const auto & cluster : clusters)
				{
					float32 d = cluster.getDistance(dataPoint);
					if (d < dist) dist = d;
				}

				if (dist > maxDist)
				{
					maxDist = dist;
					furthest = i;
				}
			}

			// Add furthest to k set
			Cluster<T> cluster;
			cluster.reset(dataPoints[furthest]);
			clusters.push_back(cluster);
		}

		return clusters;
	}
	/// @}

	/// Create MPI data type
	static void createMpiDataType()
	{
		MPI_Aint extentT; MPI_Type_extent(T::type, &extentT);

		MPI_Datatype types[]	= {T::type, MPI_FLOAT};
		int32 blockCounts[]		= {2, 1};
		MPI_Aint offsets[]		= {0, extentT};

		MPI_Type_create_struct(2, blockCounts, offsets, types, &type);
		MPI_Type_commit(&type);
	}
};

template<typename T>
MPI_Datatype Cluster<T>::type;

#include "vector.h"