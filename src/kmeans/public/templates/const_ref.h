#pragma once

/**
 * Returns type as const reference if
 * sizeof type is bigger than pointer size
 */
template<typename T, bool = (sizeof(T) > sizeof(void*))> struct ConstRef { using Type = void; };

template<typename T> struct ConstRef<T, false>	{ using Type = T; };
template<typename T> struct ConstRef<T, true>	{ using Type = const T&; };