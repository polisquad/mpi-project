#pragma once

#include "core_types.h"
#include <type_traits>

/**
 * @brief Sets value to @c true if is trivial type
 */
template<typename T>
using IsTrivial = std::is_trivial<T>;

#define IsTrivialV(T) IsTrivial<T>::value