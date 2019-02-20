#pragma once

#include "core_types.h"

/**
 * @class Singleton templates/singleton.h
 * 
 * A singleton is an object that exists only
 * once in the lifetime of an application
 */
template<class T>
class Singleton
{
public:
	/// Default constructor
	FORCE_INLINE Singleton() = default;

private:
	/// Copy constructor, removed
	Singleton(const Singleton&) = delete;

	/// Copy assignment, removed
	Singleton & operator=(const Singleton&) = delete;

public:
	/// Returns ref to global instance
	static FORCE_INLINE T & get()
	{
		static T instance;
		return instance;
	}

	/// Returns pointer to global instance
	static FORCE_INLINE T * getPtr()
	{
		return &get();
	}
};