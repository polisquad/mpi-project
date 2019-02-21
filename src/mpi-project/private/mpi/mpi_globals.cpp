#include "mpi/mpi_globals.h"

/// Global variables inital values
/// @{
bool bMPIInitialized = false;
/// @}

bool MPI::init(int32 * argc, char *** argv)
{
	if (!bMPIInitialized)
	{
		int32 status = MPI_Init(argc, argv);
		if (status != MPI_SUCCESS)
		{
			char errorMessage[MPI_MAX_ERROR_STRING];
			MPI_Error_string(status, errorMessage, nullptr);

			// @todo better error log
			printf("%s\n", errorMessage);
		}
		else
			bMPIInitialized = true;
	}

	return bMPIInitialized;
}

bool MPI::shutdown()
{
	if (bMPIInitialized)
	{
		int32 status = MPI_Finalize();
		if (status != MPI_SUCCESS)
		{
			char errorMessage[MPI_MAX_ERROR_STRING];
			MPI_Error_string(status, errorMessage, nullptr);

			// @todo better error log
			printf("%s\n", errorMessage);
		}
		else
			bMPIInitialized = false;
	}

	return !bMPIInitialized;
}

//////////////////////////////////////////////////
// Pre-defined MPI data types
//////////////////////////////////////////////////

template<>	MPI_Datatype MPI::DataType<int8>::type	= MPI_INT8_T;
template<>	MPI_Datatype MPI::DataType<int16>::type	= MPI_INT16_T;
template<>	MPI_Datatype MPI::DataType<int32>::type	= MPI_INT32_T;
template<>	MPI_Datatype MPI::DataType<int64>::type	= MPI_INT64_T;

template<>	MPI_Datatype MPI::DataType<uint8>::type		= MPI_UINT8_T;
template<>	MPI_Datatype MPI::DataType<uint16>::type	= MPI_UINT16_T;
template<>	MPI_Datatype MPI::DataType<uint32>::type	= MPI_UINT32_T;
template<>	MPI_Datatype MPI::DataType<uint64>::type	= MPI_UINT64_T;

template<>	MPI_Datatype MPI::DataType<float32>::type	= MPI_FLOAT;
template<>	MPI_Datatype MPI::DataType<float64>::type	= MPI_DOUBLE;
template<>	MPI_Datatype MPI::DataType<float128>::type	= MPI_LONG_DOUBLE;

template<>	MPI_Datatype MPI::DataType<char>::type	= MPI_CHAR;