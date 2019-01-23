#include "parallel/mpi_globals.h"

#include <unordered_map>

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

#include "parallel/mpi_device.h"

/// Map of local devices
static std::unordered_map<MPI_Comm, MPI::DeviceRef> localDeviceMap;

MPI::DeviceRef MPI::getLocalDevice(MPI_Comm communicator)
{
	auto localDeviceIt = localDeviceMap.find(communicator);
	if (localDeviceIt != localDeviceMap.end())
		return (*localDeviceIt).second;
	else
	{
		DeviceRef localDevice = new Device(communicator);
		localDeviceMap[communicator] = localDevice;

		return localDevice;
	}
}