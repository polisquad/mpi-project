#pragma once

#include "core_types.h"
#include <type_traits>

/// @brief SFINAE enable_if
template<bool Cond, typename Type = void>
using EnableIf = std::enable_if<Cond, Type>;

/// @brief SFINAE enable_if_t
template<bool Cond, typename Type = void>
using EnableIfT = std::enable_if_t<Cond, Type>;