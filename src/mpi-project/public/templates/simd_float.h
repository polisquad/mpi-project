#pragma once

#include "core_types.h"
#include "simd.h"

#if PLATFORM_ENABLE_SIMD

namespace SIMD
{	
	/// @ref Vector float specialization
	template<>
	struct Vector<float32, 8>
	{
		using Type = __m256;

	public:
		/**
		 * Load operations
		 * @{
		 */
		/// @param [in] s scalar operand
		static FORCE_INLINE Type load(float32 s) { return Type{s, s, s, s, s, s, s, s}; }
		/// @}

		/**
		 * Vector-vector arithmetic operations
		 * 
		 * @param [in] v1,v2 vector operands
		 * @return new vector
		 * @{
		 */
		static FORCE_INLINE Type add(Type v1, Type v2) { return _mm256_add_ps(v1, v2); }
		static FORCE_INLINE Type sub(Type v1, Type v2) { return _mm256_sub_ps(v1, v2); }
		static FORCE_INLINE Type div(Type v1, Type v2) { return _mm256_div_ps(v1, v2); }
		static FORCE_INLINE Type mul(Type v1, Type v2) { return _mm256_mul_ps(v1, v2); }

		/// Compute masked product and sum of two vectors
		template<uint32 mask>
		static FORCE_INLINE Type dp(Type v1, Type v2) { return _mm256_dp_ps(v1, v2, mask); }
		/// @}

		/**
		 * Vector-scalar arithmetic operations
		 * 
		 * @param [in] v vector operand
		 * @param [in] s scalar operand
		 * @return new vector
		 * @{
		 */
		static FORCE_INLINE Type add(Type v, float32 s) { return _mm256_add_ps(v, Type{s, s, s, s, s, s, s, s}); }
		static FORCE_INLINE Type sub(Type v, float32 s) { return _mm256_sub_ps(v, Type{s, s, s, s, s, s, s, s}); }
		static FORCE_INLINE Type div(Type v, float32 s) { return _mm256_div_ps(v, Type{s, s, s, s, s, s, s, s}); }
		static FORCE_INLINE Type mul(Type v, float32 s) { return _mm256_mul_ps(v, Type{s, s, s, s, s, s, s, s}); }
		/// @}

		/**
		 * Advanced element-wise arithmetic operations
		 * 
		 * @param [in] v vector operand
		 * @return new vector
		 * @{
		 */
		static FORCE_INLINE Type sqrt(Type v) { return _mm256_sqrt_ps(v); }
		///@}

		/**
		 * Convert vector to scalar
		 * 
		 * @param [in] v vector operand
		 * @returns first element of vector
		 */
		static FORCE_INLINE float32 convert(Type v)
		{
			union
			{
				Type _v;
				float32 _f;
			} out = {v};
			return out._f;
		}
	};
} // SIMD

#endif