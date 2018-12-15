#pragma once

#include "core_types.h"
#include "containers/array.h"
#include "containers/string.h"
#include "templates/enable_if.h"
#include "templates/is_trivial.h"
#include "templates/is_array.h"
#include "templates/singleton.h"

#include <mpi.h>

/// @brief Flag to indicate if MPI was initialized
extern bool bInitialized;

/// @brief Worl communicator size
extern uint32 worldSize;

namespace MPI
{
	/// @brief Global MPI initialization
	FORCE_INLINE bool init(int & argc, char ** & argv)
	{
		// Don't init again
		if (bInitialized) return true;

		if (MPI_Init(&argc, &argv) == MPI_SUCCESS)
		{
			// Get world communicator size
			int32 _;
			if (MPI_Comm_size(MPI_COMM_WORLD, &_) == MPI_SUCCESS) worldSize = _;

			// MPI initialized
			bInitialized = true;
			return true;
		}

		return false;
	}

	/// @brief Shutdown MPI interface
	FORCE_INLINE bool shutdown()
	{
		static bool bInitialized;
		if (MPI_Finalize() == MPI_SUCCESS)
		{
			bInitialized = false;
			return true;
		}

		return false;
	}

	/// @brief Returns true if MPI is initialized
	FORCE_INLINE bool isInitialized() { return bInitialized; }

	/**
	 * @class ScopeSync async/mpi.h
	 * @brief Section syncronized using a barrier
	 */
	class ScopeSync
	{
	protected:
		/// @brief Communicator
		MPI_Comm comm;

	public:
		/// @brief Default cosntructor
		FORCE_INLINE ScopeSync(MPI_Comm _comm = MPI_COMM_WORLD) : comm(_comm) {}

		/// @brief Destructor, sync
		FORCE_INLINE ~ScopeSync() { MPI_Barrier(comm); }
	};

	/**
	 * @class Device async/mpi.h
	 * @brief Describes a single mpi device
	 */
	class Device
	{
	public:
		/// @brief MPI communicator
		MPI_Comm communicator;

		/// @brief Process rank
		int32 rank;

		/// @brief Processor name
		String name;
	
	public:
		/// @brief Default-constructor, removed
		FORCE_INLINE Device(const MPI_Comm & _communicator = MPI_COMM_WORLD) : communicator(_communicator)
		{
			ASSERT(isInitialized(), "MPI not initialized");

			// Get rank and device name
			MPI_Comm_rank(communicator, &rank);

			char cname[256]; int32 cnameLength = 0;
			MPI_Get_processor_name(cname, &cnameLength);
			name = String(cname, cnameLength);
		}

		/// @brief Returns device rank
		FORCE_INLINE int32 getRank() const { return rank; }

		/// @brief Returns device name
		FORCE_INLINE const String & getName() const { return name; }

		/// @brief Returns size of communicator of this device
		FORCE_INLINE uint32 getCommSize() const
		{
			if (LIKELY(communicator == MPI_COMM_WORLD))	
				return getWorldSize();
			else
			{
				int32 result = 0;
				MPI_Comm_size(communicator, &result);
				return result;
			}
		}

		/// @brief Returns size of world communicator
		static FORCE_INLINE uint32 getWorldSize() { return worldSize; }

		/// @brief Send an object (blocking)
		/// @{
		template<typename T>
		EnableIfT<IsTrivialV(T), bool> send(const T & object, int32 dest, int32 tag = MPI_ANY_TAG);

		/// @brief Receive an object (blocking)
		template<typename T>
		EnableIfT<IsTrivialV(T), bool> receive(T & object, int32 src, int32 tag = MPI_ANY_TAG);

		/// @brief Send a buffer (send content)
		template<typename T>
		bool sendBuffer(const T * buffer, uint64 num, int32 dest, int32 tag = MPI_ANY_TAG);

		/// @brief Send a buffer (send content)
		template<typename T>
		bool receiveBuffer(T * buffer, uint64 num, int32 src, int32 tag = MPI_ANY_TAG);
	};
	typedef Device* DeviceRef;

	/**
	 * @class WorldDevice async/mpi.h
	 * @brief Device that refers to the local machine
	 */
	class WorldDevice : public Device, public Singleton<WorldDevice> {};
	typedef WorldDevice* WorldDeviceRef;
} // MPI

template<typename T>
EnableIfT<IsTrivialV(T), bool> MPI::Device::send(const T & object, int32 dest, int32 tag)
{
	// We assume we can perform a shallow copy of the object
	return MPI_Send(&object, sizeof(T), MPI_BYTE, dest, tag, communicator) == MPI_SUCCESS;
}

template<typename T>
EnableIfT<IsTrivialV(T), bool> MPI::Device::receive(T & object, int32 src, int32 tag)
{
	MPI_Status _;
	return MPI_Recv(&object, sizeof(T), MPI_BYTE, src, tag, communicator, &_) == MPI_SUCCESS;
}

template<typename T>
bool MPI::Device::sendBuffer(const T * buffer, uint64 num, int32 dest, int32 tag)
{
	return MPI_Send(buffer, num * sizeof(T), MPI_BYTE, dest, tag, communicator) == MPI_SUCCESS;
}

template<typename T>
bool MPI::Device::receiveBuffer(T * buffer, uint64 num, int32 src, int32 tag)
{
	MPI_Status _;
	return MPI_Recv(buffer, num * sizeof(T), MPI_BYTE, src, tag, communicator, &_) == MPI_SUCCESS;
}