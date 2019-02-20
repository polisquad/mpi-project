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