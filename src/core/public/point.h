#pragma once

#include "core_types.h"

/**
 * @struct Point point.h
 * @brief A 2D point mainly used for experimenting
 */
template<typename T = float32>
struct Point
{
public:
	union
	{
		/// @brief Coordinates buffer
		T data[2];

		/// @brief Single components
		/// @{
		struct
		{
			float32 x, y;
		};
		/// @}
	};

public:
	/// @brief Default-constructor
	FORCE_INLINE Point() = default;

	/// @brief Copy-constructor
	FORCE_INLINE Point(const Point<T> & p) = default;

	/// @brief Components-constructor
	FORCE_INLINE Point(T _x, T _y) : x(_x), y(_y) {}

	/// @brief Buffer-constructor
	FORCE_INLINE Point(T _data[2]) { memcpy(data, _data, sizeof(data)); }

	/// @brief Assignment-operator
	FORCE_INLINE Point & operator=(const Point<T> & p) = default;

	
	/// @brief Return squared length of point (from origin)
	FORCE_INLINE T getSquaredSize() const { return (x * x) + (y * y); }
	
	/**
	 * @brief Point/Point operators
	 * 
	 * @param [in] p other point
	 * 
	 * @return Point result of operation
	 * @{
	 */
	FORCE_INLINE Point operator+(const Point<T> & p) const { return Point(x + p.x, y + p.y); }
	FORCE_INLINE Point operator-(const Point<T> & p) const { return Point(x - p.x, y - p.y); }
	FORCE_INLINE Point operator*(const Point<T> & p) const { return Point(x * p.x, y * p.y); }
	FORCE_INLINE Point operator/(const Point<T> & p) const { return Point(x / p.x, y / p.y); }
	/// @}

	/// @brief Return squared distance between two points
	FORCE_INLINE T getSquaredDistance(const Point<T> & p) { return operator-(p).getSquaredSize(); }

	/**
	 * @brief Print point to stream
	 * 
	 * @param [in] stream output stream
	 */
	inline void print(FILE * stream = stdout);
};

/////////////////////////////////////////////////
// Float32 specialization                      //
/////////////////////////////////////////////////

template<>
void Point<float32>::print(FILE * stream)
{
	fprintf(stream, "p2(%.3f, %.3f)\n", x, y);
}