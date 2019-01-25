#pragma once

#include "coremin.h"
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
protected:
	/// Current centroid
	T centroid;

	/// Working centroid
	T workingCentroid;

	/// Working weight
	float32 workingWeight;

	/// OMP critical section
	OMP::CriticalSection mutex;

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
		ScopeLock<decltype(mutex)> _(&mutex);

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
		centroid = workingCentroid * (1.f / workingWeight);

		// Reset working set
		workingCentroid = T();
		workingWeight = 0.f;
	}

	/// Cluster initialization algorithms
	/// @{
	/// Randomly pick k centroids
	static Array<Cluster<T>, 32> initRandom(const Array<T> & dataPoints, uint32 numClusters)
	{
		printf("randommmm");
		Array<Cluster<T>> clusters(numClusters);
		const uint32 numDataPoints = dataPoints.getSize();

		// Check that num of data points is sufficient
		if (numDataPoints <= numClusters)
		{
			for (const auto & dataPoint : dataPoints)
			{
				Cluster<T> cluster;
				cluster.reset(dataPoint);
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
				uint32 idx = rand() % numDataPoints;

				for (uint32 k = 0; k < i & !bDuplicate; ++k)
					if (pick[k] == idx) bDuplicate = true;
			} while (bDuplicate);

			pick[i] = idx;
			
			Cluster<T> cluster(dataPoints[idx]);
			clusters.push(cluster);
		}

		return clusters;
	}

	/// Find k furthest centroids
	static Array<Cluster<T>, 32> initFurthest(const Array<T> & dataPoints, uint32 numClusters)
	{
		Array<Cluster<T>> clusters(numClusters);
		const uint32 numDataPoints = dataPoints.getSize();

		if (numDataPoints <= numClusters)
		{
			for (const auto & dataPoint : dataPoints)
			{
				Cluster<T> cluster(dataPoint);
				clusters.push(cluster);
			}
			
			return clusters;
		}

	#if 1
		// Randomly choose first
		{
			Cluster<T> cluster;
			cluster.reset(dataPoints[rand() % numDataPoints]);
			clusters.push(cluster);
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
			clusters.push(cluster);
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
			clusters.push(cluster);
		}

	#endif

		while (clusters.getSize() < numClusters)
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
			clusters.push(cluster);
		}

		return clusters;
	}
	/// @}
};

/**
 * Simple data point with two components
 */
template<typename T>
struct GCC_ALIGN(8) Point2
{
public:
	union
	{
		/// Data buffer
		T buffer[2];

		struct
		{
			/// Point single components
			/// @{
			T x, y;
			/// @}
		};
	};

public:
	/// Default constructor, zero initalizes
	FORCE_INLINE Point2() : buffer{0, 0} {}

	/// Component constructor
	FORCE_INLINE Point2(T _x, T _y) : buffer{_x, _y} {}

	/// Scalar constructor, implicit cast
	FORCE_INLINE Point2(T s) : buffer{s, s} {}

	/// Random access operator
	FORCE_INLINE T & operator[](uint8 i)		{ return buffer[i]; }
	FORCE_INLINE T operator[](uint8 i) const	{ return buffer[i]; }

	/// Arithmetic operators
	/// @{
	FORCE_INLINE Point2<T> operator+(const Point2<T> & p) const
	{
		return Point2<T>(x + p.x, y + p.y);
	}
	FORCE_INLINE Point2<T> operator*(float32 s) const
	{
		return Point2<T>(x * s, y * s);
	}
	/// @}

	/// Arithmetic compound assignment operators
	/// @{
	FORCE_INLINE Point2<T> & operator+=(const Point2<T> & p)
	{
		x += p.x, y += p.y;
		return *this;
	}
	FORCE_INLINE Point2<T> & operator*=(float32 s)
	{
		x *= s, y *= s;
		return *this;
	}
	/// @}

	/// Returns the distance between two points
	FORCE_INLINE float32 getDistance(const Point2<T> & p) const
	{
		const T
			dx = p.x - x,
			dy = p.y - y;

		return sqrtf(dx * dx + dy * dy);
	}

	/// Print data point
	void print(FILE * out = stdout);
};

template<>
void Point2<float32>::print(FILE * out)
{
	fprintf(out, "p2(%.3f, %.3f)\n", x, y);
}