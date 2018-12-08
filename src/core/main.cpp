#include <stdio.h>

#include "async/mpi.h"

class FixedStringMessage
{
public:
	char message[256];
	FORCE_INLINE FixedStringMessage(const char * _message)
	{
		memcpy(message, _message, 255);
		message[255] = '\0';
	}
};

int main(int argc, char ** argv)
{
	MPI::init(argc, argv);
	
	MPI::WorldDeviceRef worldDevice = MPI::WorldDevice::getPtr();
	printf("I'm local device '%s'(#%d)\n", worldDevice->getName().c_str(), worldDevice->getRank());
	if (worldDevice->getRank() > 0)
	{
		// Send message
		FixedStringMessage message("Yo bro!\n");
		worldDevice->send(&message, 0, 0);
	}
	else
	{
		MPI_Status status;
		FixedStringMessage message("");
		for (uint8 k = 1; k < worldSize; ++k)
		{
			worldDevice->receive(&message, k, 0);
			printf("%s\n", message.message);
		}
	}

	MPI::shutdownMPI();
	return 0;
}