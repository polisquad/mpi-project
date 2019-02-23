#pragma once

#include "core_types.h"
#include "is_void.h"

namespace SIMD
{
	/**
	 * @class Vector simd/vector.h
	 * 
	 * Simd vector class
	 */
	template<typename T, uint32 N>
	struct Vector
	{
		using Type = void;
	};
} // SIMD

#define hasVectorIntrinsics(T, N) !IsVoid<typename SIMD::Vector<T, N>::Type>::value

#if PLATFORM_ENABLE_SIMD

/// Include vector intrinsics
#include <immintrin.h>

namespace SIMD
{
	/// Comparison operators
	enum class Compare : int32
	{
		EQ	= _MM_CMPINT_EQ,
		NE	= _MM_CMPINT_NE,
		LT	= _MM_CMPINT_LT,
		LE	= _MM_CMPINT_LE,
		GT	= _MM_CMPINT_GT,
		GE	= _MM_CMPINT_GE,
		NLT	= _MM_CMPINT_NLT,
		NLE	= _MM_CMPINT_NLE
	};
};

#endif

//////////////////////////////////////////////////
// Specializations
//////////////////////////////////////////////////

#include "simd_float.h"
#include "simd_double.h"