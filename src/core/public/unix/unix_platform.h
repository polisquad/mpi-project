#pragma once

#include "generic/generic_platform.h"

/**
 * @struct UnixPlatformTypes unix/unix_platform.h
 * @brief Unix specific type definitions
 */
typedef struct UnixPlatformTypes : public GenericPlatformTypes
{
    // Empty
} PlatformTypes;

/////////////////////////////////////////////////
// Platform specs                              //
/////////////////////////////////////////////////

#if defined(_LINUX64) || defined(_LP64)
#define PLATFORM_64 1
#else
#define PLATFORM_64 0
#endif

#define PLATFORM_LITTLE_ENDIAN 1
#define PLATFORM_USE_PTHREADS 1

/////////////////////////////////////////////////
// Compiler attributes                         //
/////////////////////////////////////////////////

#if BUILD_DEBUG
#define FORCE_INLINE inline
#else
#define FORCE_INLINE __attribute__((always_inline)) inline
#endif
#define FORCE_NOINLINE __attribute__((noinline))
#define GCC_PACK(n) __attribute__((packed,aligned(n)))
#define GCC_ALIGN(n) __attribute__((aligned(n)))
#define FORCE_LOOP_UNROLL __attribute__((optimize("unroll-loops")))