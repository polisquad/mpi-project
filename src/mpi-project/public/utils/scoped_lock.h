#pragma once

#include "core_types.h"

/**
 * @class ScopedLock utils/scoped_lock.h
 * 
 * Acquire lock for a scoped region
 */
template<class LockT>
class ScopedLock
{
protected:
	/// Lock resource
	LockT * mutex;

public:
	/// Default constructor, acquires lock
	FORCE_INLINE ScopedLock(LockT * _mutex) : mutex(_mutex)
	{
		mutex->lock();
	}

	/// Destructor
	FORCE_INLINE ~ScopedLock()
	{
		mutex->unlock();
	}
};