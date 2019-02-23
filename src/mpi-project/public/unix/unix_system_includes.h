#pragma once

#include "core_types.h"

#include <float.h>

#if PLATFORM_USES_PTHREADS
	#include <pthread.h>
#endif

#if PLATFORM_ENABLE_SIMD
	#include <immintrin.h>
#endif