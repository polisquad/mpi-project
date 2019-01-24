#pragma once

#include "coremin.h"
#include "data.h"
#include "parallel/mpi.h"
#include "parallel/omp.h"
#include "parallel/threading.h"

#include <vector>

/// Partition data space in k clusters
template<typename T>
std::vector<std::vector<T>> clusterize(const std::vector<T> & dataPoints, uint32 numClusters)
{
	std::vector<std::vector<T>> groups(numClusters);
	MPI::DeviceRef host = MPI::getLocalDevice();

	// Generate clusters
	auto clusters = genClusters(dataPoints, numClusters);

	// Split points evenly between devices
	const uint32 numWorkers	= host->getCommSize();
	const uint32 workerId	= host->getId();
	const uint32 numDataPoints = dataPoints.size();

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
			groups[nearest].push_back(dataPoint);
		}
	}

	return groups;
}

template<typename T>
std::vector<Cluster<T>> genClusters(const std::vector<T> & dataPoints, uint32 numClusters)
{
	std::vector<Cluster<T>> clusters;
	MPI::DeviceRef host = MPI::getLocalDevice();
	bool bConverges = false;
	float32 averageDrift = 0.f;

	// Convergence options
	// @todo make this configurable, maybe a settings struct
	const float32 convergenceFactor = 0.0625f; // Reasonable values around 0.1f
	const uint32 maxIterations = 100;

	//////////////////////////////////////////////////
	// Clusters initialization
	//////////////////////////////////////////////////

	/// @todo options to choose cluster initialization method
	if (host->isMaster())
	{
		// Initialize clusters
		// @todo parallel initialization?
		clusters = Cluster<T>::initFurthest(dataPoints, numClusters);

		// Send initialization to slaves
		host->broadcast(&clusters[0], clusters.size());
	}
	else
	{
		clusters.resize(numClusters);

		// Wait for master to initialize clusters
		host->receiveBroadcast(&clusters[0], numClusters, 0);
	}

	// Split points evenly between devices
	const uint32 numWorkers	= host->getCommSize();
	const uint32 workerId	= host->getId();
	const uint32 numDataPoints = dataPoints.size();

	for (uint32 epoch = 0; epoch < maxIterations /* & !bConverges */; ++epoch)
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
					std::vector<Cluster<T>> remoteClusters(numClusters);
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
		std::vector<Cluster<T>> remoteClusters(numWorkers * numClusters);
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