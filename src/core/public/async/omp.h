#pragma once

#include "core_types.h"
#include <omp.h>

namespace OMP
{
    /**
     * @class CriticalSection async/omp.h
     * @brief A critical section is a section that may be accessed by a single process
     */
    class CriticalSection
    {
    protected:
        /// @brief OpenMP mutex
        omp_lock_t mutex;

    public:
        /// @brief Default-constructor
        FORCE_INLINE CriticalSection()
        { omp_init_lock(&mutex); }

        /// @brief Lock critical section
        FORCE_INLINE void lock()
        { omp_set_lock(&mutex); }

        /// @brief Unlock critical section
        FORCE_INLINE void unlock()
        { omp_unset_lock(&mutex); }
    };

    /**
     * @class ScopeLock async/omp.h
     * @brief Automatically lock and unlock a scope
     */
    class ScopeLock
    {
    protected:
        /// @brief This critical section
        CriticalSection *mutex;

    public:
        /// @brief Default-constructor, lock
        FORCE_INLINE ScopeLock(CriticalSection *_mutex) : mutex(_mutex)
        { mutex->lock(); }

        /// @brief Destructor, unlock
        FORCE_INLINE ~ScopeLock()
        { mutex->unlock(); }
    };
} // OMP