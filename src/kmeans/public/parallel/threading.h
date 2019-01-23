#pragma once

/**
 * A scope lock locks the section of code
 * in which it's declared
 */
template<typename LockT>
class ScopeLock
{
protected:
	LockT * mutex;

public:
	/// Default constructor
	FORCE_INLINE ScopeLock(LockT * _mutex) :
		mutex(_mutex)
	{
		// Lock immediately
		mutex->lock();
	}

	// Destroy and release lock
	FORCE_INLINE ~ScopeLock()
	{
		mutex->unlock();
	}
};