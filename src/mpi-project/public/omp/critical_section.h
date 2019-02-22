#pragma once

#include "core_types.h"

#include <omp.h>

namespace OMP
{
	/**
	 * @class CriticalSection omp/critical_section.h
	 * 
	 * Wrapper for an openmp lock
	 */
	class CriticalSection
	{
	protected:
		/// OpenMP mutex
		omp_lock_t mutex;

	public:
		/// Default constructor
		FORCE_INLINE CriticalSection()
		{
			// Init mutex
			omp_init_lock(&mutex);
		}

		/// Destructor
		FORCE_INLINE ~CriticalSection()
		{
			// Destroy mutex
			omp_destroy_lock(&mutex);
		}

		/// Acquire region lock
		FORCE_INLINE void lock()
		{
			omp_set_lock(&mutex);
		}

		/// Release region lock
		FORCE_INLINE void unlock()
		{
			omp_unset_lock(&mutex);
		}
	};
} // OMP
