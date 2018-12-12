#include "async/mpi.h"
#include "templates/enable_if.h"

/// @brief MPI globals
/// @{
bool bInitialized = false;
uint32 worldSize = 0;
/// @}