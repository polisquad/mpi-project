#pragma once

#include "coremin.h"

#define DEFAULT_MAX_SIZE 8

/**
 * A variable-length vector
 */
template<typename T, uint32 N>
struct Vector
{
public:
	/// MPI data type
	static MPI_Datatype type;

protected:
	/// Data buffer
	T buffer[N];

	/// Vector size
	uint32 size;

public:
	/// Default cosntructor
	explicit FORCE_INLINE Vector() :
		buffer{},
		size(N) {}

	/// Initialization constructor
	explicit FORCE_INLINE Vector(uint32 _size, const T * values = nullptr) :
		size(_size > N ? N : _size)
	{
		// Initialize buffer
		if (values != nullptr)
		{
			const sizet bufferSize = size * sizeof(T);
			memcpy(buffer, values, bufferSize);
		}
	}

	/// Scalar constructor
	FORCE_INLINE Vector(uint32 _size, typename ConstRef<T>::Type s) :
		size(_size > N ? N : _size)
	{
		// Initialize buffer
		for (uint32 i = 0; i < size; ++i)
			new (buffer + i) T(s);
	}

	/// Returns vector size
	FORCE_INLINE uint32 getSize() const { return size; }

	/// Random access operator
	FORCE_INLINE T &		operator[](uint32 i)		{ return buffer[i]; }
	FORCE_INLINE const T &	operator[](uint32 i) const	{ return buffer[i]; }

	/// Resize vector
	FORCE_INLINE void resize(uint32 _size) { size = _size > N ? N : _size; }

protected:
	/**
	 * Operations on buffer, class usage only
	 * 
	 * All operations modify buffer b1
	 * 
	 * @param [in] b,b1,b2 buffer operands
	 * @param [in] s scalar operand
	 * @return ref to b1
	 * @{
	 */
	static FORCE_INLINE T * addBuffer(T * b1, const T * b2, uint32 size)
	{
		// Manual unroll
		T * out = b1;
		for (; size >= 4; size -= 4, b1 += 4, b2 += 4)
		{
			b1[0] += b2[0],
			b1[1] += b2[1],
			b1[2] += b2[2],
			b1[3] += b2[3];
		}
		for (; size >= 2; size -= 2, b1 += 2, b2 += 2)
		{
			b1[0] += b2[0],
			b1[1] += b2[1];
		}
		if (size)
			b1[0] += b2[0];

		return out;
	}

	static FORCE_INLINE T * subBuffer(T * b1, const T * b2, uint32 size)
	{
		// Manual unroll
		T * out = b1;
		for (; size >= 4; size -= 4, b1 += 4, b2 += 4)
		{
			b1[0] -= b2[0],
			b1[1] -= b2[1],
			b1[2] -= b2[2],
			b1[3] -= b2[3];
		}
		for (; size >= 2; size -= 2, b1 += 2, b2 += 2)
		{
			b1[0] -= b2[0],
			b1[1] -= b2[1];
		}
		if (size)
			b1[0] -= b2[0];

		return out;
	}

	static FORCE_INLINE T * mulBuffer(T * b1, T s, uint32 size)
	{
		// Manual unroll
		T * out = b1;
		for (; size >= 4; size -= 4, b1 += 4)
		{
			b1[0] *= s,
			b1[1] *= s,
			b1[2] *= s,
			b1[3] *= s;
		}
		for (; size >= 2; size -= 2, b1 += 2)
		{
			b1[0] *= s,
			b1[1] *= s;
		}
		if (size)
			b1[0] *= s;

		return out;
	}
	/// @}

	/// Horizontal sum buffer
	static FORCE_INLINE T haddBuffer(const T * b, uint32 size)
	{
		T out = T(0);
		for (; size >= 4; size -= 4, b += 4)
			out += b[0] + b[1] + b[2] + b[3];
		for (; size >= 2; size -= 2, b += 2)
			out += b[0] + b[1];
		if (size)
			out += b[0];
		
		return out;
	}

	/// Just a square ...
	static FORCE_INLINE T square(T s) { return s * s; }

	/// Compute horizontal sum of difference
	static inline T normBuffer(const T * b1, const T * b2, uint32 size)
	{
		T out = T(0);
		for (; size >= 4; size -= 4, b1 += 4, b2 += 4)
		{
			out += square(b1[0] - b2[0]),
			out += square(b1[1] - b2[1]),
			out += square(b1[2] - b2[2]),
			out += square(b1[3] - b2[3]);
		}
		for (; size >= 2; size -= 2, b1 += 2, b2 += 2)
		{
			out += square(b1[0] - b2[0]),
			out += square(b1[1] - b2[1]);
		}
		if (size)
			out += square(b1[0] - b2[0]);
		
		return out;
	}

public:
	/**
	 * Vector-vector compound assignments
	 * 
	 * @param [in] v vector operand
	 * @return self
	 * @{
	 */
	FORCE_INLINE Vector<T, N> & operator+=(const Vector<T, N> & v)
	{
		// Use minimum size (no overflow)
		const uint32 minSize = this->size < v.size ? this->size : v.size;
		addBuffer(this->buffer, v.buffer, minSize);

		return *this;
	}
	FORCE_INLINE Vector<T, N> & operator-=(const Vector<T, N> & v)
	{
		// Use minimum size (no overflow)
		const uint32 minSize = this->size < v.size ? this->size : v.size;
		subBuffer(this->buffer, v.buffer, minSize);

		return *this;
	}
	/// @}

	/**
	 * Vector-scalar compound assignments
	 * 
	 * @param [in] s scalar operand
	 * @return self
	 */
	FORCE_INLINE Vector<T, N> & operator*=(T s)
	{
		mulBuffer(this->buffer, s, this->size);
		return *this;
	}

	/**
	 * Vector-vector arithmetic operators
	 * 
	 * @param [in] v vector operand
	 * @return new vector
	 * @{
	 */
	FORCE_INLINE Vector<T, N> operator+(const Vector<T, N> & v) const
	{
		// Use minimum size (no overflow)
		const uint32 minSize = this->size < v.size ? this->size : v.size;
		Vector<T, N> result(*this);
		addBuffer(result.buffer, v.buffer, minSize);

		return result;
	}
	FORCE_INLINE Vector<T, N> operator-(const Vector<T, N> & v) const
	{
		// Use minimum size (no overflow)
		const uint32 minSize = this->size < v.size ? this->size : v.size;
		Vector<T, N> result(*this);
		subBuffer(result.buffer, v.buffer, minSize);

		return result;
	}
	/// @}

	/**
	 * Vector-scalar arithmetic operators
	 * 
	 * @param [in] s scalar operand
	 * @return new vector
	 * @{
	 */
	FORCE_INLINE Vector<T, N> operator*(T s) const
	{
		Vector<T, N> result(*this);
		mulBuffer(result.buffer, s, this->size);

		return result;
	}

	/// Returns distance between two vectors
	FORCE_INLINE float32 getDistance(const Vector<T, N> & v) const
	{
		// Use minimum size (no overflow)
		const uint32 minSize = this->size < v.size ? this->size : v.size;
		return sqrtf(normBuffer(this->buffer, v.buffer, minSize));
	}

	/// Create MPI data type
	static FORCE_INLINE void createMpiDataType()
	{
		MPI_Datatype types[1]	= {MPI::DataType<T>::value};
		int32 blockCounts[1]	= {N};
		MPI_Aint offsets[1]		= {0};

		MPI_Type_create_struct(1, blockCounts, offsets, types, &type);
		MPI_Type_commit(&type);
	}
};

template<typename T, uint32 N>
MPI_Datatype Vector<T, N>::type;

/// Default max size
template<typename T>
using Vec = Vector<T, DEFAULT_MAX_SIZE>;