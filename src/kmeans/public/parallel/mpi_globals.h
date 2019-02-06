#pragma once

#include "core_types.h"

/// Library include
#include <mpich/mpi.h>

/// Global MPI variables and flags
/// @{
extern bool bMPIInitialized;
/// @}

/// Execute and catch error macro
#define tryError(stmt)\
{\
	int32 __status = stmt;\
	if (__status != MPI_SUCCESS)\
	{\
		char __errorMessage[MPI_MAX_ERROR_STRING];\
		MPI_Error_string(__status, __errorMessage, nullptr);\
		\
		printf("%s\n", __errorMessage);\
	}\
}

#define tryStatus(stmt, status)\
int32 status = stmt;\
if (status != MPI_SUCCESS)\
{\
	char __errorMessage[MPI_MAX_ERROR_STRING];\
	MPI_Error_string(status, __errorMessage, nullptr);\
	\
	printf("%s\n", __errorMessage);\
}\

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

	/// Returns ref to local device
	class Device * getLocalDevice(MPI_Comm communicator = MPI_COMM_WORLD);

	/**
	 * Sets value to appropariate MPI data type enum
	 * @{
	 */
	template<typename T>					struct DataType						{ enum {value = MPI_BYTE}; };
	template<>								struct DataType<int32>				{ enum {value = MPI_INT}; };
	template<>								struct DataType<float32>			{ enum {value = MPI_FLOAT}; };
	template<>								struct DataType<float64>			{ enum {value = MPI_DOUBLE}; };
	template<>								struct DataType<uint32>				{ enum {value = MPI_UNSIGNED}; };
	/// @}
}
