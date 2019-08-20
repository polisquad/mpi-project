#pragma once

#include "core_types.h"
#include "array.h"
#include "templates/const_ref.h"
#include "mpi/mpi_globals.h"

/**
 * @class Cluster containers/cluster.h
 * 
 * Contains the current cluster centroid and
 * a working cluster setup
 */
template<typename T>
class GCC_ALIGN(alignof(T)) Cluster : public MPI::DataType<Cluster<T>>
{
protected:
	/// Cluster current centroid
	T centroid;

	/// Cluster working centroid
	T workingCentroid;

	/// Working weight
	float32 weight;

public:
	/// Default constructor
	FORCE_INLINE Cluster(typename ConstRef<T>::Type _centroid = T()) :
		centroid(_centroid),
		workingCentroid(T()),
		weight(0.f) {}

	/// Get distance between point and cluster centroid
	FORCE_INLINE float32 getDistance(typename ConstRef<T>::Type p) const
	{
		return centroid.getDistance(p);
	}

	/// Get distance between two clusters (centroids)
	FORCE_INLINE float32 getDistance(const Cluster & other) const
	{
		return centroid.getDistance(other.centroid);
	}

	/// Add weight to working centroid
	FORCE_INLINE void addWeight(typename ConstRef<T>::Type p, float32 w)
	{
		workingCentroid += p, weight += w;
	}

	/// Fuse with other cluster
	FORCE_INLINE Cluster & fuse(const Cluster & other)
	{
		workingCentroid += other.workingCentroid;
		weight += other.weight;

		return *this;
	}

	/// Commit working centroid
	FORCE_INLINE Cluster & commit()
	{
		// Update current centroid
		if (weight)
		{
			centroid = workingCentroid * (1.f / weight);

			// Reset working centroid
			workingCentroid = T();
			weight = 0.f;
		}

		return *this;
	}

	/// Cluster initialization algorithms
	/// @{
	/// Randomly pick k centroids
	static Array<Cluster> initRandom(const Array<T> & dataPoints, uint32 numClusters)
	{
		Array<Cluster> clusters;
		const uint32 numDataPoints = dataPoints.getCount();

		// Init clusters
		clusters.reserve(numClusters);

		// Check that num of data points is sufficient
		if (numDataPoints <= numClusters)
		{
			for (const auto & dataPoint : dataPoints)
			{
				Cluster cluster(dataPoint);
				clusters.push(cluster);
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
				idx = rand() % numDataPoints;

				for (uint32 k = 0; k < i && !bDuplicate; ++k)
					if (pick[k] == idx) bDuplicate = true;
			} while (bDuplicate);

			pick[i] = idx;
			
			Cluster cluster(dataPoints[idx]);
			clusters.push(cluster);
		}

		delete[] pick;
		return clusters;
	}

	/// Find k furthest centroids
	static Array<Cluster> initFurthest(const Array<T> & dataPoints, uint32 numClusters)
	{
		Array<Cluster> clusters;
		const uint32 numDataPoints = dataPoints.getCount();

		// Make space for clusters
		clusters.reserve(numClusters);

		if (numDataPoints <= numClusters)
		{
			for (const auto & dataPoint : dataPoints)
			{
				Cluster cluster(dataPoint);
				clusters.push(cluster);
			}
			
			return clusters;
		}

	#if 1
		// Randomly choose first
		{
			Cluster cluster(dataPoints[rand() % numDataPoints]);
			clusters.push(cluster);
		}
	#elif 0
		{
			Cluster cluster;

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
			clusters.push(cluster);
		}	
	#else
		{
			Cluster cluster;

			// Choose average
			T average = dataPoints[0];
			uint32 i = 1;
			for (; i < numDataPoints; ++i)
				average += dataPoints[i];

			cluster.reset(average * (1.f / i));
			clusters.push(cluster);
		}

	#endif

		while (clusters.getCount() < numClusters)
		{
			uint32 furthest = -1;
			float32 maxDist = 0.f;

			for (uint32 i = 0; i < numDataPoints; ++i)
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
			Cluster cluster(dataPoints[furthest]);
			clusters.push(cluster);
		}

		return clusters;
	}
	/// @}

	//////////////////////////////////////////////////
	// MPI Interface
	//////////////////////////////////////////////////
	
	/// Creates the MPI datatype, if not already created
	static FORCE_INLINE MPI_Datatype createMpiType()
	{
		const int32 blockSize[] = {2, 1};
		const MPI_Aint blockDisplacement[] = {0, offsetof(Cluster, weight)};
		const MPI_Datatype blockType[] = {MPI::DataType<T>::type, MPI::DataType<float32>::type};

		MPI_Type_create_struct(2, blockSize, blockDisplacement, blockType, &Cluster::type);

		// Align bounds to type size
		MPI_Type_create_resized(Cluster::type, 0, sizeof(Cluster), &Cluster::type);

		// Commit type
		MPI_Type_commit(&Cluster::type);
		return Cluster::type;
	}

	//////////////////////////////////////////////////
	// Debug
	//////////////////////////////////////////////////
	
#if BUILD_DEBUG
	/// Get cluster centroid
	/// @{
	FORCE_INLINE T &		getCurrentCentroid()		{ return centroid; }
	FORCE_INLINE const T &	getCurrentCentroid() const	{ return centroid; }
	/// @}
#endif
};