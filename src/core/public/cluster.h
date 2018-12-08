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

public:
	/// @brief Default-constructor
	FORCE_INLINE Cluster() : centroid(), workingCentroid(centroid), weight(0) {}

	/// @brief Get distance from centroid
	FORCE_INLINE auto getDistance(const T & elem) const { return centroid.getDistance(elem); }

	/// @brief Add weight
	FORCE_INLINE void addWeight(const T & elem) { OMP::ScopeLock _; workingCentroid = workingCentroid + elem; ++weight; }

	/// @brief Fuse with another cluster
	/// @{
	FORCE_INLINE Cluster & operator+=(const Cluster<T> & cluster) { weight += cluster.weight; workingCentroid += workingCentroid; return *this; }
	FORCE_INLINE Cluster & fuse(const Cluster<T> & cluster) { return operator+=(cluster); }
	/// @}

	/// @brief Commit changes to cluster
	FORCE_INLINE void commit() { centroid = workingCentroid / weight, workingCentroid = centroid, weight = 0; }
};