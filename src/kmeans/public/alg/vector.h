#pragma once

#include "coremin.h"

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

	/// Random access operator
	FORCE_INLINE T &		operator[](uint32 i)		{ return buffer[i]; }
	FORCE_INLINE const T &	operator[](uint32 i) const	{ return buffer[i]; }

	/// Resize vector
	FORCE_INLINE void resize(uint32 _size) { size = _size > N ? size : _size; }
};

#include "templates/simd.h"
// Todo, force std::vector to align correctly
// and remove unaligned load/store

/**
 * Variable-length vector type compliant
 * with cluster data type requirements
 */
template<typename T, uint32 N, bool = hasVectorIntrinsics(T, 8)>
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
struct GCC_ALIGN(32) Vector<T, N, true> : public VectorBase<T, N>
{
public:
	/// Vector intrinsics type
	using VecOps	= Simd::Vector<T, 8>;
	using VecT		= typename VecOps::Type;

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
	static inline T * addBuffer(T * b1, const T * b2, uint32 size)
	{
		switch (size) // It's faster, piss off!!
		{
			case 0:
				return b1;
			case 1:
				b1[0] += b2[0];
				break;
			case 2:
				b1[0] += b2[0], b1[1] += b2[1];
				break;
			case 3:
				b1[0] += b2[0], b1[1] += b2[1], b1[2] += b2[2];
				break;
			case 4:
				b1[0] += b2[0], b1[1] += b2[1], b1[2] += b2[2], b1[3] += b2[3];
				break;
			default:
				if (size < 8)
				{
					// Recursive call
					addBuffer(b1, b2, 4);
					addBuffer(b1 + 4, b2 + 4, size - 4);
				}
				else
				{
					// Use vector intrinsics
					uint32 offset = 0;
					for (; size >= 8; size -= 8, offset += 8)
						VecOps::storeu(b1 + offset, VecOps::add(VecOps::loadu(b1 + offset), VecOps::loadu(b2 + offset)));

					// Recursive call
					addBuffer(b1 + offset, b2 + offset, size);
				}
				break;
		}
		
		return b1;
	}

	static inline T * subBuffer(T * b1, const T * b2, uint32 size)
	{
		switch (size)
		{
			case 0:
				return b1;
			case 1:
				b1[0] -= b2[0];
				break;
			case 2:
				b1[0] -= b2[0], b1[1] -= b2[1];
				break;
			case 3:
				b1[0] -= b2[0], b1[1] -= b2[1], b1[2] -= b2[2];
				break;
			case 4:
				b1[0] -= b2[0], b1[1] -= b2[1], b1[2] -= b2[2], b1[3] -= b2[3];
				break;
			default:
				if (size < 8)
				{
					// Recursive call
					subBuffer(b1, b2, 4);
					subBuffer(b1 + 4, b2 + 4, size - 4);
				}
				else
				{
					// Use vector intrinsics
					uint32 offset = 0;
					for (; size >= 8; size -= 8, offset += 8)
						VecOps::storeu(b1 + offset, VecOps::sub(VecOps::loadu(b1 + offset), VecOps::loadu(b2 + offset)));

					// Recursive call
					subBuffer(b1 + offset, b2 + offset, size);
				}
				break;
		}
		
		return b1;
	}

	static inline T * mulBuffer(T * b1, T s, uint32 size)
	{
		switch (size)
		{
			case 0:
				return b1;
			case 1:
				b1[0] *= s;
				break;
			case 2:
				b1[0] *= s, b1[1] *= s;
				break;
			case 3:
				b1[0] *= s, b1[1] *= s, b1[2] *= s;
				break;
			case 4:
				b1[0] *= s, b1[1] *= s, b1[2] *= s, b1[3] *= s;
				break;
			default:
				if (size < 8)
				{
					// Recursive call
					mulBuffer(b1, s, 4);
					mulBuffer(b1 + 4, s, size - 4);
				}
				else
				{
					// Use vector intrinsics
					uint32 offset = 0;
					for (; size >= 8; size -= 8, offset += 8)
						VecOps::storeu(b1 + offset, VecOps::mul(VecOps::loadu(b1 + offset), VecOps::load(s)));

					// Recursive call
					mulBuffer(b1 + offset, s, size);
				}
				break;
		}
		
		return b1;
	}
	/// @}

	/// Horizontal sum buffer
	static inline T haddBuffer(const T * b, uint32 size)
	{
		if (size <= 0) return T(0);

		switch (size)
		{
			case 1:
				return b[0];
			case 2:
				return b[0] + b[1];
			case 3:
				return b[0] + b[1] + b[2];
			case 4:
				return b[0] + b[1] + b[2] + b[3];		
			default:
				if (size < 16)
					return haddBuffer(b, 4) + haddBuffer(b + 4, size - 4);
				else
				{
					// Use vector intrinsics
					T temp[8];
					VecT accum = VecOps::loadu(b);

					uint32 offset = 8;
					size -= 8;
					for (; size >= 8; size -= 8, offset += 8)
						accum = VecOps::add(accum, VecOps::loadu(b + offset));
					
					VecOps::storeu(temp, accum);

					return haddBuffer(temp, 8) + haddBuffer(b + offset, size);
				}
		}
	}

	/// Just a square ...
	static FORCE_INLINE T square(T s) { return s * s; }

	/// Compute horizontal sum of difference
	static inline T normBuffer(const T * b1, const T * b2, uint32 size)
	{
		if (size <= 0) return T(0);

		switch (size)
		{
			case 1:
				return square(b1[0] - b2[0]);
			case 2:
				return square(b1[0] - b2[0]) + square(b1[1] - b2[1]);
			case 3:
				return square(b1[0] - b2[0]) + square(b1[1] - b2[1]) + square(b1[2] - b2[2]);
			case 4:
				return square(b1[0] - b2[0]) + square(b1[1] - b2[1]) + square(b1[2] - b2[2]) + square(b1[3] - b2[3]);		
			default:
				if (size < 16)
					return normBuffer(b1, b2, 4) + normBuffer(b1 + 4, b2 + 4, size - 4);
				else
				{
					// Use vector intrinsics
					T temp[8];
					VecT accum = VecOps::sub(VecOps::loadu(b1), VecOps::loadu(b2));
					accum = VecOps::mul(accum, accum);

					uint32 offset = 8;
					size -= 8;
					for (; size >= 8; size -= 8, offset += 8)
					{
						VecT diff = VecOps::sub(VecOps::loadu(b1 + offset), VecOps::loadu(b2 + offset));
						accum = VecOps::add(accum, VecOps::mul(diff, diff));
					}
					
					VecOps::storeu(temp, accum);

					return haddBuffer(temp, 8) + normBuffer(b1 + offset, b2 + offset, size);
				}
		}
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