#pragma once

/// Set all other platforms to 0
#ifndef PLATFORM_WINDOWS
	#define PLATFORM_WINDOWS 0
#endif
#ifndef PLATFORM_APPLE
	#define PLATFORM_APPLE 0
#endif
#ifndef PLATFORM_UNIX
	#define PLATFORM_UNIX 0
#endif

/// Platform defines
#include "generic/generic_platform.h"
#if PLATFORM_WINDOWS
	#include "windows/windows_platform.h"
#elif PLATFORM_APPLE
	#include "apple/apple_platform.h"
#elif PLATFORM_UNIX
	#include "unix/unix_platform.h"
#else
	#error Unkown platform
#endif

/// System includes
#include "generic/generic_system_includes.h"
#if PLATFORM_WINDOWS
	#include "windows/windows_system_includes.h"
#elif PLATFORM_APPLE
	#include "apple/apple_system_includes.h"
#elif PLATFORM_UNIX
	#include "unix/unix_system_includes.h"
#else
	#error Unkown platform
#endif

/////////////////////////////////////////////////
// Platform specs                              //
/////////////////////////////////////////////////

#ifndef PLATFORM_64
	#define PLATFORM_64 0
#endif
#ifndef PLATFORM_32
	#define PLATFORM_32 !PLATFORM_64
#endif
#ifndef PLATFORM_LITTLE_ENDIAN
	#define PLATFORM_LITTLE_ENDIAN 0
#endif

/////////////////////////////////////////////////
// Compiler attributes                         //
/////////////////////////////////////////////////

#ifndef FORCE_INLINE
	#define FORCE_INLINE inline
#endif
#ifndef LIKELY
	#define LIKELY(condition) (condition)
#endif
#ifndef UNLIKELY
	#define UNLIKELY(condition) (condition)
#endif

/// Method modifiers

#ifndef CONSTEXPR
	#define CONSTEXPR constexpr
#endif

/// Alignment

#ifndef GCC_PACK
	#define GCC_PACK(n)
#endif
#ifndef GCC_ALIGN
	#define GCC_ALIGN(n)
#endif

/// Miscs

#ifndef ASSERT
	#define ASSERT(cond, text) assert(cond && text)
#endif

/// Expose global plaftorm type definitions

/// @brief Integer types
/// @{
typedef PlatformTypes::byte		byte;
typedef PlatformTypes::int8		int8;
typedef PlatformTypes::int16	int16;
typedef PlatformTypes::int32	int32;
typedef PlatformTypes::int64	int64;

typedef PlatformTypes::ubyte	ubyte;
typedef PlatformTypes::uint8	uint8;
typedef PlatformTypes::uint16	uint16;
typedef PlatformTypes::uint32	uint32;
typedef PlatformTypes::uint64	uint64;
/// @}

/// @brief Floating-point types
/// @{
typedef PlatformTypes::float32	float32;
typedef PlatformTypes::float64	float64;
typedef PlatformTypes::float128	float128;
/// @}

/// @brief Pointer types
/// @{
typedef PlatformTypes::intP		intP;
typedef PlatformTypes::uintP	uintP;

typedef PlatformTypes::ssizet	ssizet;
typedef PlatformTypes::sizet	sizet;
/// @}