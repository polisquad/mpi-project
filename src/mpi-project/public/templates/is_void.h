#pragma once

#include "core_types.h"

/**
 * Sets value to true if type is void
 */
template<typename T>	struct IsVoid		{ enum {value = false}; };
template<>				struct IsVoid<void>	{ enum {value = true}; };