#pragma once

#include "core_types.h"
#include "containers/array.h"

/**
 * @brief Sets value member to @c true if
 * type is an @ref Array
 */
template<typename T> struct IsArray				{ enum {value = false}; };
template<typename T> struct IsArray<Array<T>>	{ enum {value = true}; };

#define IsArrayV(T) IsArray<T>::value

/**
 * @brief Sets Type to the array type
 */
template<typename T> struct ArrayType			{};
template<typename T> struct ArrayType<Array<T>>	{ using Type = T; };

#define ArrayTypeT(T) typename ArrayType<T>::Type

/**
 * @brief Sets value member to @c true if
 * type of the array is T
 */
template<typename ArrayT, typename T>	struct IsArrayOfType				{ enum {value = false}; };
template<typename T>					struct IsArrayOfType<Array<T>, T>	{ enum {value = true}; };

#define IsArrayOfTypeV(ArrayT, T) IsArrayOfType<ArrayT, T>::value