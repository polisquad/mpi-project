#pragma once

#include "coremin.h"
#include "data.h"
#include "utils/command_line.h"
#include "parallel/mpi.h"
#include "parallel/omp.h"
#include "parallel/threading.h"

#include <vector>

/// Partition data space in k clusters
template<typename T>
Array<Array<T>, alignof(T)> clusterize(const Array<T> & dataPoints, uint32 numClusters)
{
	Array<Array<T>, alignof(T)> groups(numClusters);
	MPI::DeviceRef host = MPI::getLocalDevice();
	const uint32 numDataPoints = dataPoints.getSize();

	// Init groups
	for (uint32 k = 0; k < numClusters; ++k)
		groups.push(Array<T>(numDataPoints / numClusters + 1));

	// Generate clusters
	auto clusters = genClusters(dataPoints, numClusters);

	// Split points evenly between devices
	const uint32 numWorkers	= host->getCommSize();
	const uint32 workerId	= host->getId();

	// Lock for each group
	std::vector<OMP::CriticalSection> groupLocks(numClusters);

	#pragma omp parallel for
	for (uint32 i = workerId; i < numDataPoints; i += numWorkers)
	{
		const T & dataPoint = dataPoints[i];

		uint32 nearest = 0;
		float32 minDist = clusters[0].getDistance(dataPoint);

		for (uint32 k = 1; k < numClusters; ++k)
		{
			float32 dist = clusters[k].getDistance(dataPoint);
			if (dist < minDist) minDist = dist, nearest = k;
		}

		// Aggregate to nearest cluster
		{
			ScopeLock<OMP::CriticalSection> _(&groupLocks[nearest]);
			groups[nearest].push(dataPoint);
		}
	}

	return groups;
}

template<typename T>
Array<Cluster<T>> genClusters(const Array<T> & dataPoints, uint32 numClusters)
{
	Array<Cluster<T>> clusters(numClusters);
	MPI::DeviceRef host = MPI::getLocalDevice();
	bool bConverges = false;
	float32 averageDrift = 0.f;

	// Convergence options
	// Read from command line
	float32 convergenceFactor = 0.0625f; // Reasonable values around 0.1f
	uint32 maxIterations = 256;
	
	CommandLine::get().getValue("--convergence-factor", convergenceFactor);
	CommandLine::get().getValue("--max-iter", maxIterations);

	// Split points evenly between devices
	const uint32 numWorkers	= host->getCommSize();
	const uint32 workerId	= host->getId();
	const uint32 numDataPoints = dataPoints.getSize();

	//////////////////////////////////////////////////
	// Clusters initialization
	//////////////////////////////////////////////////

	/// @todo options to choose cluster initialization method
	if (workerId == 0)
	{
		// Initialize clusters
		// @todo parallel initialization?
		std::string initMethod;
		CommandLine::get().getValue("--init", initMethod);

		if (initMethod == "random")
			clusters = Cluster<T>::initRandom(dataPoints, numClusters);
		else
			clusters = Cluster<T>::initFurthest(dataPoints, numClusters);

		// Send initialization to slaves
		host->broadcast(&clusters[0], numClusters);
	}
	else
	{
		// Wait for master to initialize clusters
		host->receiveBroadcast(&clusters[0], numClusters, 0);
	}

	for (uint32 epoch = 0; epoch < maxIterations & !bConverges; ++epoch)
	{
		// Save current cluster states for convergence check
		auto prevClusters = clusters;

		//////////////////////////////////////////////////
		// Cluster aggregation
		//////////////////////////////////////////////////

		// A single iteration
		#pragma omp parallel
		{
			// Thread local copy
			// No need for critical sections
			auto threadClusters = clusters;

			#pragma omp for
			for (uint32 i = workerId; i < numDataPoints; i += numWorkers)
			{
				const T & dataPoint = dataPoints[i];

				uint32 nearest = 0;
				float32 minDist = threadClusters[0].getDistance(dataPoint);

				for (uint32 k = 1; k < numClusters; ++k)
				{
					float32 dist = threadClusters[k].getDistance(dataPoint);
					if (dist < minDist) minDist = dist, nearest = k;
				}

				// Aggregate to nearest cluster
				threadClusters[nearest].addWeight(dataPoint);
			}
			
			// Fuse with master clusters
			for (uint32 k = 0; k < numClusters; ++k)
				clusters[k].fuse(threadClusters[k]);
		}
		
		//////////////////////////////////////////////////
		// MPI synchronization
		//////////////////////////////////////////////////
		
		/**
		 * Two approaches:
		 * 
		 * - The centralized approach: each slave sends its partial
		 *   updates to the master host (id = 0). The master node
		 *   gathers the updates, fuses them together and broadcast
		 *   the result.
		 * 
		 * - The distributed approach: no slave-master, each node
		 *   broadcasts its partial updates, receives all other updates
		 *   and fuses them by itself. No need to send back the results.
		 *   Final clusters should be consistent as long as everyone
		 *   received the update.
		 * 
		 * @todo time them and choose
		 * 
		 * The first approach is clearly faster on a single machine,
		 * I should test this with different hosts
		 */

		#if 1
		if (host->isMaster())
		{
			// Receive partial updates from slaves
			for (uint32 i = 0; i < numWorkers; ++i)
				if (i != workerId)
				{
					Array<Cluster<T>> remoteClusters(numClusters);
					host->receive(&remoteClusters[0], numClusters, i, epoch);

					// Fuse with this
					for (uint32 k = 0; k < numClusters; ++k)
						clusters[k].fuse(remoteClusters[k]);
				}
			
			// Commit updates
			float32 maxDrift = 0.f;
			for (uint32 k = 0; k < numClusters; ++k)
			{
				clusters[k].commit();

				const float32 drift = clusters[k].getDistance(prevClusters[k]);
				if (drift > maxDrift) maxDrift = drift;
			}

			// @todo How to make this relative?
			// Update average drift
			averageDrift = (averageDrift * epoch + maxDrift) / (epoch + 1);
			bConverges = maxDrift <= averageDrift * convergenceFactor;
			
			// Broadcast results
			host->broadcast<bool>(bConverges);
			host->broadcast(&clusters[0], numClusters);
		}
		else
		{
			// Send partial update to master
			host->send(&clusters[0], numClusters, 0, epoch);

			// Wait for master to reply
			host->receiveBroadcast(bConverges, 0);
			host->receiveBroadcast(&clusters[0], numClusters, 0);
		}
		#else
		// Gather all updates
		Array<Cluster<T>> remoteClusters(numWorkers * numClusters);
		host->allgather(&clusters[0], numClusters, &remoteClusters[0], numClusters);
		
		// Commit udpates
		#pragma omp parallel for
		for (uint32 k = 0; k < numClusters; ++k)
		{
			for (uint32 w = 0; w < numWorkers; ++w)
				if (w != workerId)
				{
					clusters[k].fuse(remoteClusters[w * numWorkers + k]);
				}

			clusters[k].commit();
		}

		// @todo check terminating condition
		#endif
	}

	return clusters;
}