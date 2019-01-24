#pragma once

/// Sets flag to true if is void type
template<typename T>	struct IsVoid 		{ enum {value = false}; };
template<>				struct IsVoid<void> { enum {value = true}; };