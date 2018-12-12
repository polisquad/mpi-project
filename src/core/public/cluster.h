#pragma once

#include "core_types.h"
#include "containers/array.h"
#include "async/omp.h"

template<typename T>
class Cluster
{
protected:
	/// @brief Current centroid of the group
	T centroid;

	/// @brief Working centroid of the group
	T workingCentroid;

	/// @brief Current total weight
	uint64 weight;

	/// @brief Cluster usage guard
	OMP::CriticalSection usageGuard;

public:
	/// @brief Default-constructor
	FORCE_INLINE Cluster() : centroid(), workingCentroid(centroid), weight(0) {}

	/// @brief Get usage guard mutex
	FORCE_INLINE OMP::CriticalSection & getUsageGuard() { return usageGuard; }

	/// @brief Get distance from centroid
	FORCE_INLINE auto getDistance(const T & elem) const { return centroid.getDistance(elem); }

	/// @brief Add weight
	FORCE_INLINE void addWeight(const T & elem) { workingCentroid += elem; ++weight; }

	/// @brief Compare two clusters
	FORCE_INLINE bool operator==(const Cluster<T> & cluster) { centroid == cluster.centroid; }
	FORCE_INLINE bool operator!=(const Cluster<T> & cluster) { centroid != cluster.centroid; }

	/// @brief Fuse with another cluster
	/// @{
	FORCE_INLINE Cluster & operator+=(const Cluster<T> & cluster)
	{
		if (cluster.weight > 0)
		{
			weight += cluster.weight;
			workingCentroid += cluster.workingCentroid;
		}
		return *this;
	}
	FORCE_INLINE Cluster & fuse(const Cluster<T> & cluster) { return operator+=(cluster); }
	FORCE_INLINE friend Cluster fuse(const Cluster<T> & a, const Cluster<T> & b) { return a.fuse(b); }
	/// @}

	/// @brief Commit changes to cluster
	FORCE_INLINE void commit()
	{
		if (weight > 0)
			centroid = workingCentroid / weight,
			workingCentroid = centroid,
			weight = 1;
	}

#if BUILD_DEBUG
	FORCE_INLINE void printDebug()
	{
		printf("weight: %llu, centroid: ", weight);
		centroid.print();
	}
#endif
};