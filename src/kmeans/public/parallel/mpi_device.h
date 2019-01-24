#pragma once

#include "core_types.h"
#include "mpi_globals.h"
#include "templates/const_ref.h"

namespace MPI
{
	class Device
	{
	protected:
		/// Global communicator used by this device
		MPI_Comm communicator;

		/// Process id in the community
		int32 id;

		/// Device name
		std::string name;

		/// Stats on messages
		uint32 numSent, numReceived;

	public:
		/// Default constructor
		FORCE_INLINE Device() : Device(MPI_COMM_WORLD) {}

		/// Initializer constructor
		FORCE_INLINE Device(MPI_Comm _communicator) :
			communicator(_communicator),
			name(MPI_MAX_PROCESSOR_NAME, '\0'),
			numSent(0),
			numReceived(0)
		{
			// Get valid id
			tryError(MPI_Comm_rank(communicator, &id))


			// Try to get local device name
			int32 length = 0;
			tryError(MPI_Get_processor_name(&name[0], &length))
			name.resize(length + 1);
		}

		/// Returns device communicator size
		FORCE_INLINE int32 getCommSize() const { return MPI::getCommSize(communicator); }

		/// Returns commmunicator name
		FORCE_INLINE std::string getCommName() const {return MPI::getCommName(communicator); }

		/// Returns device id
		FORCE_INLINE int32 getId() const { return id; }

		/// Returns device name
		FORCE_INLINE std::string getName() const { return name; }

		/// Return true if is master
		FORCE_INLINE bool isMaster() const { return id == 0; }

		/// Send algorithms
		/// @{
		
		/// Sends a trivial object to dest
		template<typename T>
		FORCE_INLINE bool send(typename ConstRef<T>::Type obj, int32 dest, int32 tag)
		{
			tryStatus(MPI_Send(&obj, sizeof(T), MPI_BYTE, dest, tag, communicator), status)

			if (status == MPI_SUCCESS)
			{
				// Increment number of messages sent
				++numSent;
				return true;
			}

			return false;
		}
		template<typename T>
		FORCE_INLINE bool send(typename ConstRef<T>::Type obj, int32 dest) { return send(obj, dest, numSent); }

		/// Sends a static buffer to dest
		template<typename T>
		FORCE_INLINE bool send(const T * buffer, uint32 count, int32 dest, int32 tag)
		{
			tryStatus(MPI_Send(buffer, count * sizeof(T), MPI_BYTE, dest, tag, communicator), status)

			if (status == MPI_SUCCESS)
			{
				// Increment number of messages sent
				++numSent;
				return true;
			}

			return false;
		}
		template<typename T>
		FORCE_INLINE bool send(const T * buffer, uint32 count, int32 dest) { return send(buffer, count, dest, numSent); }

		/// Broadcast a trivial payload to all hosts listening on communicator
		template<typename T>
		FORCE_INLINE bool broadcast(typename ConstRef<T>::Type obj)
		{
			tryStatus( MPI_Bcast((T*)&obj, sizeof(T), MPI_BYTE, id, communicator), status)

			if (status == MPI_SUCCESS)
			{
				// Increment number of messages sent
				++numSent;
				return true;
			}

			return false;
		}

		/// Broadcast a static buffer to all hosts listening on communicator
		template<typename T>
		FORCE_INLINE bool broadcast(const T * buffer, uint32 count)
		{
			tryStatus(MPI_Bcast((T*)buffer, count * sizeof(T), MPI_BYTE, id, communicator), status)

			if (status == MPI_SUCCESS)
			{
				// Increment number of messages sent
				numSent += getCommSize();
				return true;
			}

			return false;
		}

		/// @}

		/// Receive methods
		/// @{
		
		/// Receives a static buffer
		template<typename T>
		FORCE_INLINE bool receive(T * buffer, uint32 count, int32 src, int32 tag)
		{
			MPI_Status _;
			tryStatus(MPI_Recv(buffer, count * sizeof(T), MPI_BYTE, src, tag, communicator, &_), status)

			if (status == MPI_SUCCESS)
			{
				// Increment number of messages sent
				++numReceived;
				return true;
			}

			return false;
		}
		template<typename T>
		FORCE_INLINE bool receive(T * buffer, uint32 count, int32 src) { return receive(buffer, count, src, numReceived); }

		/// Receives a trivial object broadcast
		template<typename T>
		FORCE_INLINE bool receiveBroadcast(T & obj, uint32 root = 0)
		{
			tryStatus(MPI_Bcast(&obj, sizeof(T), MPI_BYTE, root, communicator), status)

			if (status == MPI_SUCCESS)
			{
				// Increment number of messages sent
				++numReceived;
				return true;
			}

			return false;
		}

		/// Receives a static buffer broadcast
		template<typename T>
		FORCE_INLINE bool receiveBroadcast(T * buffer, uint32 count, uint32 root = 0)
		{
			tryStatus(MPI_Bcast(buffer, count * sizeof(T), MPI_BYTE, root, communicator), status)

			if (status == MPI_SUCCESS)
			{
				// Increment number of messages sent
				++numReceived;
				return true;
			}

			return false;
		}
		/// @}

		/// Multi-broadcast method
		/// Every node sends its data, data is collected
		/// and redistributed
		/// @see http://mpitutorial.com/tutorials/mpi-scatter-gather-and-allgather/
		template<typename T>
		FORCE_INLINE bool allgather(const T * sendBuffer, uint32 sendCount, T * receiveBuffer, uint32 receiveCount)
		{
			tryStatus(MPI_Allgather(sendBuffer, sendCount * sizeof(T), MPI_BYTE, receiveBuffer, receiveCount * sizeof(T), MPI_BYTE, communicator), status)

			if (status == MPI_SUCCESS)
			{
				// Increment number of messages sent
				const uint32 commSize = getCommSize();
				numSent		+= commSize;
				numReceived	+= commSize;
				return true;
			}

			return false;
		}
	};

	/// Device reference type
	using DeviceRef = Device*;
}