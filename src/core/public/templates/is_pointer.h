#pragma once

#include "core_types.h"
#include <type_traits>

/**
 * @brief Sets the const member value to @c true
 * if type is a pointer type
 */
template<typename T>
using IsPointer = std::is_pointer<T>;

#define IsPointerV(T) IsPointer<T>::value