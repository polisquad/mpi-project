#pragma once

/// Build macros coming from CMake
/// Set to zero undefined

#ifndef SGL_BUILD_DEBUG
	#define SGL_BUILD_DEBUG 0
#endif
#ifndef SGL_BUILD_DEVELOPMENT
	#define SGL_SGL_BUILD_DEVELOPMENT 0
#endif
#ifndef SGL_BUILD_RELEASE
	#define SGL_BUILD_RELEASE 0
#endif

/// Include platform core definitions
#include "hal/platform.h"

/// Include the C runtime libraries
#include "hal/platform_crt.h"