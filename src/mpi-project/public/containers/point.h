#pragma once

#include "core_types.h"
#include "templates/const_ref.h"
#include "mpi/mpi_globals.h"

#define POINT_MAX_SIZE 8

/**
 * @class Point containers/point.h
 * 
 * A multi-dimensional point
 */
template<typename T, uint32 N>
class Point : public MPI::DataType<Point<T, N>>
{
protected:
	/// Point data
	T data[N];

	/// Actual size of the point
	uint32 size;

public:
	/// Default constructor, zero initialize
	FORCE_INLINE Point(uint32 _size = N) :
		data{},
		size(_size < N ? _size : N) {}
	
	/// Random access operator
	/// @{
	FORCE_INLINE T &		operator[](uint32 i)		{ return data[i]; }
	FORCE_INLINE const T &	operator[](uint32 i) const	{ return data[i]; }
	/// @}

protected:
	/// Computes the squared distance between two scalars
	static FORCE_INLINE T dist2(typename ConstRef<T>::Type s1, typename ConstRef<T>::Type s2)
	{
		const T d = s1 - s2;
		return d * d;
	}

	/// Computes squared norm of buffer
	static FORCE_INLINE T norm2(const T * b1, uint32 n)
	{
		T out = T();
		const T * it = b1;

		for (; it <= b1 + n - 8; it += 8)
			out += (it[0] * it[0]) + (it[1] * it[1]) + (it[2] * it[2]) + (it[3] * it[3]) + (it[4] * it[4]) + (it[5] * it[5]) + (it[6] * it[6]) + (it[7] * it[7]);
		for (; it <= b1 + n - 4; it += 4)
			out += (it[0] * it[0]) + (it[1] * it[1]) + (it[2] * it[2]) + (it[3] * it[3]);
		for (; it <= b1 + n - 2; it += 2)
			out += (it[0] * it[0]) + (it[1] * it[1]);
		if (it != b1 + n)
			out += (it[0] * it[0]);
		
		return norm2;
	}

	/**
	 * Buffer-buffer arithm operations
	 * 
	 * Stores result in first buffer operand
	 * 
	 * @param [in] b1 dest buffer
	 * @param [in] b2 source buffer
	 * @param [in] n buffers length
	 * @{
	 */
	static FORCE_INLINE void add(T * b1, const T * b2, uint32 n)
	{
				T * i1 = b1;
		const	T * i2 = b2;

		for (; i1 <= b1 + n - 8; i1 += 8, i2 += 8)
			i1[0] += i2[0], i1[1] += i2[1],
			i1[2] += i2[2], i1[3] += i2[3],
			i1[4] += i2[4], i1[5] += i2[5],
			i1[6] += i2[6], i1[7] += i2[7];
		for (; i1 <= b1 + n - 4; i1 += 4, i2 += 4)
			i1[0] += i2[0], i1[1] += i2[1],
			i1[2] += i2[2], i1[3] += i2[3];
		for (; i1 <= b1 + n - 2; i1 += 2, i2 += 2)
			i1[0] += i2[0], i1[1] += i2[1];
		if (i1 != b1 + n)
			i1[0] += i2[0];
	}

	static FORCE_INLINE void sub(T * b1, const T * b2, uint32 n)
	{
				T * i1 = b1;
		const	T * i2 = b2;

		for (; i1 <= b1 + n - 8; i1 += 8, i2 += 8)
			i1[0] -= i2[0], i1[1] -= i2[1],
			i1[2] -= i2[2], i1[3] -= i2[3],
			i1[4] -= i2[4], i1[5] -= i2[5],
			i1[6] -= i2[6], i1[7] -= i2[7];
		for (; i1 <= b1 + n - 4; i1 += 4, i2 += 4)
			i1[0] -= i2[0], i1[1] -= i2[1],
			i1[2] -= i2[2], i1[3] -= i2[3];
		for (; i1 <= b1 + n - 2; i1 += 2, i2 += 2)
			i1[0] -= i2[0], i1[1] -= i2[1];
		if (i1 != b1 + n)
			i1[0] -= i2[0];
	}

	static FORCE_INLINE void mul(T * b1, const T * b2, uint32 n)
	{
				T * i1 = b1;
		const	T * i2 = b2;

		for (; i1 <= b1 + n - 8; i1 += 8, i2 += 8)
			i1[0] *= i2[0], i1[1] *= i2[1],
			i1[2] *= i2[2], i1[3] *= i2[3],
			i1[4] *= i2[4], i1[5] *= i2[5],
			i1[6] *= i2[6], i1[7] *= i2[7];
		for (; i1 <= b1 + n - 4; i1 += 4, i2 += 4)
			i1[0] *= i2[0], i1[1] *= i2[1],
			i1[2] *= i2[2], i1[3] *= i2[3];
		for (; i1 <= b1 + n - 2; i1 += 2, i2 += 2)
			i1[0] *= i2[0], i1[1] *= i2[1];
		if (i1 != b1 + n)
			i1[0] *= i2[0];
	}

	static FORCE_INLINE void div(T * b1, const T * b2, uint32 n)
	{
				T * i1 = b1;
		const	T * i2 = b2;

		for (; i1 <= b1 + n - 8; i1 += 8, i2 += 8)
			i1[0] /= i2[0], i1[1] /= i2[1],
			i1[2] /= i2[2], i1[3] /= i2[3],
			i1[4] /= i2[4], i1[5] /= i2[5],
			i1[6] /= i2[6], i1[7] /= i2[7];
		for (; i1 <= b1 + n - 4; i1 += 4, i2 += 4)
			i1[0] /= i2[0], i1[1] /= i2[1],
			i1[2] /= i2[2], i1[3] /= i2[3];
		for (; i1 <= b1 + n - 2; i1 += 2, i2 += 2)
			i1[0] /= i2[0], i1[1] /= i2[1];
		if (i1 != b1 + n)
			i1[0] /= i2[0];
	}

	static FORCE_INLINE T dist2(const T * b1, const T * b2, uint32 n)
	{
		T out = T();
		const T
			* i1 = b1,
			* i2 = b2;

		for (; i1 <= b1 + n - 8; i1 += 8, i2 += 8)
			out += dist2(i1[0], i2[0]) + dist2(i1[1], i2[1]) + dist2(i1[2], i2[2]) + dist2(i1[3], i2[3]) + dist2(i1[3], i2[3]) + dist2(i1[5], i2[5]) + dist2(i1[4], i2[4]) + dist2(i1[7], i2[7]);
		for (; i1 <= b1 + n - 4; i1 += 4, i2 += 4)
			out += dist2(i1[0], i2[0]) + dist2(i1[1], i2[1]) + dist2(i1[2], i2[2]) + dist2(i1[3], i2[3]);
		for (; i1 <= b1 + n - 2; i1 += 2, i2 += 2)
			out += dist2(i1[0], i2[0]) + dist2(i1[1], i2[1]);
		if (i1 != b1 + n)
			out += dist2(i1[0], i2[0]);
		
		return out;
	}
	/// @}

	/**
	 * Buffer-scalar operations
	 * 
	 * Stores result in buffer operand
	 * 
	 * @param [in] b buffer operand
	 * @param [in] s scalar operand
	 * @{
	 */
	static FORCE_INLINE void mul(T * b, typename ConstRef<T>::Type s, uint32 n)
	{
		T * it = b;

		for (; it <= b + n - 8; it += 8)
			it[0] *= s, it[1] *= s,
			it[2] *= s, it[3] *= s,
			it[4] *= s, it[5] *= s,
			it[6] *= s, it[7] *= s;
		for (; it <= b + n - 4; it += 4)
			it[0] *= s, it[1] *= s,
			it[2] *= s, it[3] *= s;
		for (; it <= b + n - 2; it += 2)
			it[0] *= s, it[1] *= s;
		if (it != b + n)
			it[0] *= s;
	}
	/// @}

public:
	/// Get point squared size
	T getSquaredSize() const
	{
		return norm2(data, size);
	}

	/// Get point size
	FORCE_INLINE float32 getSize() const
	{
		return ::sqrtf((float32)getSquaredSize());
	}

	/// Get squared distance between two points
	T getSquaredDistance(const Point p) const
	{
		return dist2(data, p.data, size < p.size ? size : p.size);
	}

	/// Get distance between two points
	FORCE_INLINE float32 getDistance(const Point p) const
	{
		return sqrtf((float32)getSquaredDistance(p));
	}

	/**
	 * Point-point compound assignments
	 * 
	 * @param [in] p point operand
	 * @return self
	 * @{
	 */
	Point & operator+=(const Point & p)
	{
		// Use min size
		size = size < p.size ? size : p.size;
		add(data, p.data, size);

		return *this;
	}
	/// @}

	/**
	 * Point-scalar compound assignments
	 * 
	 * @param [in] s scalar operand
	 * @return self
	 * @{
	 */
	Point & operator*=(typename ConstRef<T>::Type s)
	{
		mul(data, s, size);
		return *this;
	}
	/// @}

	/**
	 * Point-point arithmetic operations
	 * 
	 * @param [in] p point operand
	 * @return new point
	 * @{
	 */
	Point operator+(const Point p) const
	{
		Point r(*this);

		// Use min size
		r.size = size < p.size ? size : p.size;
		add(r.data, p.data, r.size);

		return r;
	}
	/// @}

	/**
	 * Point-scalar arithmetic operations
	 * 
	 * @param [in] s scalar operand
	 * @return new point
	 * @{
	 */
	Point operator*(typename ConstRef<T>::Type s)
	{
		Point r(*this);
		
		mul(r.data, s, r.size);
		return r;
	}
	/// @}

	/**
	 * Print point
	 * 
	 * @param out out stream
	 */
	FORCE_INLINE void print(FILE * out = stdout);

	//////////////////////////////////////////////////
	// MPI Interface
	//////////////////////////////////////////////////
	
	/// Creates the MPI datatype, if not already created
	static FORCE_INLINE MPI_Datatype createMpiType()
	{
		const int32 blockSize[] = {N};
		const MPI_Aint blockDisplacement[] = {0};
		const MPI_Datatype blockType[] = {MPI::DataType<T>::type};

		MPI_Type_create_struct(1, blockSize, blockDisplacement, blockType, &Point::type);

		// Commit type
		MPI_Type_commit(&Point::type);
		return Point::type;
	}
};

template<>
void Point<float32, POINT_MAX_SIZE>::print(FILE * out)
{
	uint32 i = 0;

	fprintf(out, "p(");
	for (; i < size - 1; ++i)
		fprintf(out, "%f,", data[i]); 
	fprintf(out, "%f)\n", data[i]);
}

/// Default point data type
using point = Point<float32, POINT_MAX_SIZE>;