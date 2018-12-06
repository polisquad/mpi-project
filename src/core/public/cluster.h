#pragma once

#include "core_types.h"
#include "containers/array.h"

template<typename T>
class Cluster
{
protected:
	/// @brief Current centroid of the group
	T centroid;

	/// @brief Working centroid of the group
	T nextCentroid;

	/// @brief Current total weight
	uint64 weight;

public:
	/// @brief Default-constructor
	FORCE_INLINE Cluster() : centroid(), nextCentroid(centroid), weight(0) {}

	/// @brief Get distance from centroid
	FORCE_INLINE auto getDistance(const T & elem) { return elem >> centroid; }

	/// @brief Add weight
	FORCE_INLINE void addWeight(const T & elem) { nextCentroid = (weight * nextCentroid + elem) / (weight + 1); ++weight; }

	/// @brief Commit changes to cluster
	FORCE_INLINE void commit() { centroid = nextCentroid, weight = 0; }
};