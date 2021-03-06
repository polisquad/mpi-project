#pragma once

#include "core_types.h"

/**
 * Computes the min of two const values
 */
template<int64 A, int64 B>
struct Min { enum {value = A < B ? A : B}; };