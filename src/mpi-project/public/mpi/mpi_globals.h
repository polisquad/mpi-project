#pragma once

#include "core_types.h"

/// Library include
#include <mpich/mpi.h>

/// Global MPI variables and flags
/// @{
extern bool bMPIInitialized;
/// @}

namespace MPI
{
	/// Initializes MPI
	bool init(int32 * argc = nullptr, char *** argv = nullptr);

	/// Shutdown MPI
	bool shutdown();

	/// Returns true if MPI was initialized
	FORCE_INLINE bool wasInit() { return bMPIInitialized; }

	/// Returns communicator size if communicator is valid
	FORCE_INLINE int32 getCommSize(MPI_Comm communicator = MPI_COMM_WORLD)
	{
		int32 size = 0;
		const int32 status = MPI_Comm_size(communicator, &size);
		return status == MPI_SUCCESS ? size : 0;
	}

	/// Returns communicator name if valid communicator
	FORCE_INLINE std::string getCommName(MPI_Comm communicator = MPI_COMM_WORLD)
	{
		int32 length = 0;
		std::string name(MPI_MAX_OBJECT_NAME, '\0');

		// Get communicator name
		const int32 status = MPI_Comm_get_name(communicator, &name[0], &length);
		name.resize(length + 1);

		return status == MPI_SUCCESS ? name : "n/a";
	}

	/// Return MPI time
	FORCE_INLINE float64 time()
	{
		return MPI_Wtime();
	}

	/// Return MPI tick
	FORCE_INLINE float64 tick()
	{
		return MPI_Wtick();
	}

	/**
	 * @struct DataType mpi/mpi_globals.h
	 * 
	 * Templated MPI data types
	 */
	template<typename T>
	struct DataType
	{
		/// MPI data type
		static MPI_Datatype type;
	};

	/**
	 * @struct ScopedTimer mpi/mpi_globals.h
	 * 
	 * Time a scoped region and print result
	 */
	struct ScopedTimer
	{
	protected:
		/// Start time
		float64 startTime;

	public:
		/// Default constructor
		FORCE_INLINE ScopedTimer() : startTime(time()) {}

		/// Destructor
		FORCE_INLINE ~ScopedTimer()
		{
			printf("elapsed time: %.3fs\n", time() - startTime);
		}
	};
} /// MPI

//////////////////////////////////////////////////
// MPI::DataType
//////////////////////////////////////////////////

template<typename T>
MPI_Datatype MPI::DataType<T>::type = MPI_AINT;

template<>	MPI_Datatype MPI::DataType<int8>::type;
template<>	MPI_Datatype MPI::DataType<int16>::type;
template<>	MPI_Datatype MPI::DataType<int32>::type;
template<>	MPI_Datatype MPI::DataType<int64>::type;

template<>	MPI_Datatype MPI::DataType<uint8>::type;
template<>	MPI_Datatype MPI::DataType<uint16>::type;
template<>	MPI_Datatype MPI::DataType<uint32>::type;
template<>	MPI_Datatype MPI::DataType<uint64>::type;

template<>	MPI_Datatype MPI::DataType<float32>::type;
template<>	MPI_Datatype MPI::DataType<float64>::type;
template<>	MPI_Datatype MPI::DataType<float128>::type;

template<>	MPI_Datatype MPI::DataType<char>::type;