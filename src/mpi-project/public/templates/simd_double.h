#pragma once

#include "core_types.h"
#include "simd.h"

#if PLATFORM_ENABLE_SIMD

namespace SIMD
{	
	/// @ref Vector double specialization
	template<>
	struct Vector<float64, 4>
	{
		using Type = __m256;

	public:

	};
} // SIMD

#endif