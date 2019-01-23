#pragma once

#include "core_types.h"

#include <omp.h>

namespace OMP
{
	/**
	 * Use this when the critical directive won't suffice
	 */
	struct CriticalSection
	{
	protected:
		/// The omp lock resource
		omp_lock_t mutex;

	public:
		/// Default constructor
		FORCE_INLINE CriticalSection()
		{
			// Initialize lock
			omp_init_lock(&mutex);
		}

		/// Destructor, destory mutex
		FORCE_INLINE ~CriticalSection()
		{
			omp_destroy_lock(&mutex);
		}

		/// Lock mutex
		FORCE_INLINE void lock()
		{
			omp_set_lock(&mutex);
		}

		/// Unlock mutex
		FORCE_INLINE void unlock()
		{
			omp_unset_lock(&mutex);
		}
	};
} // OMP