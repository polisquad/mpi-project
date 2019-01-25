#pragma once

#include "coremin.h"

#define DEFAULT_MAX_SIZE 8

/**
 * A variable-length vector
 */
template<typename T, uint32 N>
struct VectorBase
{
protected:
	/// Data buffer
	T buffer[N];

	/// Vector size
	uint32 size;

public:
	/// Default cosntructor
	explicit FORCE_INLINE VectorBase() :
		buffer{},
		size(N) {}

	/// Initialization constructor
	explicit FORCE_INLINE VectorBase(uint32 _size, const T * values = nullptr) :
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
	FORCE_INLINE VectorBase(uint32 _size, typename ConstRef<T>::Type s) :
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
};

#include "templates/simd.h"

/**
 * Variable-length vector type compliant
 * with cluster data type requirements
 */
template<typename T, uint32 N, bool = hasVectorIntrinsics(T, 4) & hasVectorIntrinsics(T, 8)>
struct Vector : public VectorBase<T, N>
{
public:
	/// Inherit base class constructors
	using VectorBase<T, N>::VectorBase;
};

/**
 * Vector intrisics specialization
 */
template<typename T, uint32 N>
struct GCC_PACK(32) Vector<T, N, true> : public VectorBase<T, N>
{
public:
	/// Vector intrinsics type
	using VecOps	= Simd::Vector<T, 4>;
	using DVecOps	= Simd::Vector<T, 8>;
	using VecT		= typename VecOps::Type;
	using DVecT		= typename DVecOps::Type;

public:
	/// Inherit base class constructors
	using VectorBase<T, N>::VectorBase;

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
		uint32 offset = 0;
		for (; size >= 8; size -= 8, offset += 8)
			DVecOps::store(b1 + offset, DVecOps::add(DVecOps::load(b1 + offset), DVecOps::load(b2 + offset)));
		
		for (; size >= 4; size -= 4, offset += 4)
			VecOps::store(b1 + offset, VecOps::add(VecOps::load(b1 + offset), VecOps::load(b2 + offset)));
		
		for (; size > 0; --size, ++offset)
			b1[offset] += b2[offset];


		return b1;
	}

	static FORCE_INLINE T * subBuffer(T * b1, const T * b2, uint32 size)
	{
		uint32 offset = 0;
		for (; size >= 8; size -= 8, offset += 8)
			DVecOps::store(b1 + offset, DVecOps::sub(DVecOps::load(b1 + offset), DVecOps::load(b2 + offset)));
		
		for (; size >= 4; size -= 4, offset += 4)
			VecOps::store(b1 + offset, VecOps::sub(VecOps::load(b1 + offset), VecOps::load(b2 + offset)));
		
		for (; size > 0; --size, ++offset)
			b1[offset] -= b2[offset];
		
		return b1;
	}

	static FORCE_INLINE T * mulBuffer(T * b1, T s, uint32 size)
	{
		uint32 offset = 0;
		for (; size >= 8; size -= 8, offset += 8)
			DVecOps::store(b1 + offset, DVecOps::mul(DVecOps::load(b1 + offset), DVecOps::load(s)));
		
		for (; size >= 4; size -= 4, offset += 4)
			VecOps::store(b1 + offset, VecOps::mul(VecOps::load(b1 + offset), VecOps::load(s)));
		
		for (; size > 0; --size, ++offset)
			b1[offset] *= s;
		
		return b1;
	}
	/// @}

	/// Horizontal sum buffer
	static FORCE_INLINE T haddBuffer(const T * b, uint32 size)
	{
		T out = T(0);
		uint32 offset = 0;
		for (; size >= 4; size -= 4, offset += 4)
			out += b[offset] + b[offset + 1] + b[offset + 2] + b[offset + 3];
		
		for (; size > 0; --size, ++offset)
			out += b[offset];
		
		return out;
	}

	/// Just a square ...
	static FORCE_INLINE T square(T s) { return s * s; }

	/// Compute horizontal sum of difference
	static inline T normBuffer(const T * b1, const T * b2, uint32 size)
	{
		T out = T(0);
		uint32 offset = 0;
		for (; size >= 4; size -= 4, offset += 4)
			out += square(b1[offset] - b2[offset]),
			out += square(b1[offset + 1] - b2[offset + 1]),
			out += square(b1[offset + 2] - b2[offset + 2]),
			out += square(b1[offset + 3] - b2[offset + 3]);
		
		for (; size > 0; --size, ++offset)
			out += square(b1[offset] - b2[offset]);
		
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
};

/// Default max size
template<typename T>
using Vec = Vector<T, DEFAULT_MAX_SIZE>;