#ifndef SGL_KMEANS_H
#define SGL_KMEANS_H

#include "core_types.h"
#include "containers/array.h"
#include "cluster.h"

namespace KMeans
{
	/// @brief Enumerate available cluster initialization methods
	enum EClusterInitialization : uint8
	{
		Random,
		Furthest
	};

	/**
	 * @brief Generates k clusters from a given dataset
	 * 
	 * @param [in]	dataset		input dataset
	 * @param [in]	numClusters	number of clusters
	 * @param [in]	maxIter		maximum number of iterations
	 * 
	 * @return Generated clusters
	 */
	template<typename DataT>
	Array<Cluster<DataT>> genClusters(
		const Array<DataT> & dataset,
		uint32 numClusters,
		uint32 maxIter = -1,
		EClusterInitialization initMethod = EClusterInitialization::Random
	);

	/**
	 * @brief Compute membership of each data point
	 * 
	 * @param [in]	dataset		input dataset
	 * @param [in]	clusters	available clusters
	 * 
	 * @return Array of membership
	 */
	template<typename DataT>
	Array<uint32> computeMembership(const Array<DataT> & dataset, const Array<Cluster<DataT>> & clusters);
} // KMeans

namespace ClusterInitMethods
{
	/// @brief Initialize clusters with radom data points
	template<typename DataT>
	void random(Array<Cluster<DataT>> & clusters, const Array<DataT> & dataset)
	{
		const uint32 datasetSize = dataset.size();

		for (auto & cluster : clusters)
		{
			const uint32 i = rand() % datasetSize;
			cluster.addWeight(dataset[i]);
			cluster.commit();
		}
	}

	/// @brief Initialize cluster with k furthest data points
	template<typename DataT>
	void furthest(Array<Cluster<DataT>> & clusters, const Array<DataT> & dataset)
	{
		Array<DataT> centroids;
		const uint32 numClusters = clusters.size();
		centroids.reserve(numClusters);

		// Find first randomly
		/// @todo Random or smallest?
		const uint32 datasetSize = dataset.size();
		centroids.push_back(dataset[rand() % datasetSize]);
		clusters[0].addWeight(centroids[0]);
		clusters[0].commit();

		for (uint32 k = 1; k < numClusters; ++k)
		{
			float32 maxDist = 0.f;
			uint32 furthestIdx = -1;

			for (uint32 i = 0; i < datasetSize; ++i)
			{
				const auto & dp = dataset[i];

				// First find closest between furtehst
				float32 minDist = centroids[0].getDistance(dp);

				for (uint32 k = 1; k < centroids.size(); ++k)
				{
					const float32 dist = centroids[k].getDistance(dp);
					minDist = dist < minDist ? dist : minDist;
				}

				// Now, is this distance greater than maxDist?
				if (minDist > maxDist) maxDist = minDist, furthestIdx = i;
			}

			// Add point to centroids array
			centroids.push_back(dataset[furthestIdx]);
			
			// Add also to clusters
			clusters[k].addWeight(centroids[k]);
			clusters[k].commit();
		}
	}

	template<typename DataT>
	FORCE_INLINE void init(Array<Cluster<DataT>> & clusters, const Array<DataT> & dataset, KMeans::EClusterInitialization initMethod = KMeans::EClusterInitialization::Random)
	{
		switch (initMethod)
		{
			case KMeans::EClusterInitialization::Random:
				random(clusters, dataset);
				break;
			
			case KMeans::EClusterInitialization::Furthest:
				furthest(clusters, dataset);
				break;

			default:
				// Method not implemented
				ASSERT(false, "method not implemented");
				break;
		}
	}
} // ClusterInitMethods

template<typename DataT>
Array<Cluster<DataT>> KMeans::genClusters(const Array<DataT> & dataset, uint32 numClusters, uint32 maxIter, EClusterInitialization initMethod)
{
	// Out clusters
	Array<Cluster<DataT>> clusters(numClusters, Cluster<DataT>());

	// MPI must be already initialize, we let
	// master node decide the initial setup and
	// broadcasts it to all other nodes
	ASSERT(MPI::isInitialized(), "MPI must be initialized");
	const MPI::DeviceRef device = MPI::WorldDevice::getPtr();

	//////////////////////////////////////////////////
	// Setup
	//////////////////////////////////////////////////
	
	if (device->getRank() == 0)
	{		
		// Initialize clusters
		ClusterInitMethods::init(clusters, dataset, initMethod);

		// Send initial setup to other nodes
		/// @todo Replace with broadcast method
		for (uint32 w = 1; w < device->getCommSize(); ++w)
			device->sendBuffer(&clusters[0], numClusters, w, 0x0fffffff);
	}
	else
	{
		// Receive initial setup from master
		device->receiveBuffer(&clusters[0], numClusters, 0, 0x0fffffff);
	}

	//////////////////////////////////////////////////
	// K-Means iterations
	//////////////////////////////////////////////////

	// Split work among nodes
	const uint32 datasetSize	= dataset.size();
	const uint32 offset			= device->getRank();
	const uint32 step			= device->getCommSize();
	
	/// Flag to check if algorithm converges
	uint8 bFinalize = 0;

	/// Epoch counter, stop algorithm if exceeds max no. of iterations
	uint32 epoch = 0;

	while (bFinalize == 0 & epoch < maxIter)
	{
		// Create a copy of the current cluster state
		// We are using it later to check convergence
		const Array<Cluster<DataT>> prevState(clusters);

		#pragma omp parallel
		{
			// Each thread works on a local copy of the clusters
			// to avoid costly locks and synchronization issues
			Array<Cluster<DataT>> localState(clusters);

			#pragma omp for
			for (uint32 i = offset; i < datasetSize; i += step)
			{
				// Current working data point
				const auto & dataPoint = dataset[i];

				// Find closest cluster
				float32 minDist = localState[0].getDistance(dataPoint);
				uint32 minIdx = 0;

				for (uint32 k = 1; k < numClusters; ++k)
				{
					// Update if closer
					const float32 dist = localState[k].getDistance(dataPoint);
					if (dist < minDist) minDist = dist, minIdx = k;
				}

				// Add weight to closest cluster
				localState[minIdx].addWeight(dataPoint);
			}

			// Fuse thread local state with shared state
			for (uint32 k = 0; k < numClusters; ++k)
			{
				auto cluster = clusters[k];
				{
					// Lock shared state, avoids overlapping updates
					OMP::ScopeLock _(&cluster.getUsageGuard());
					cluster.fuse(localState[k]);
				}
			}
		}

		//////////////////////////////////////////////////
		// MPI synchronize
		//////////////////////////////////////////////////
		
		if (device->getRank() == 0)
		{
			// Master node receives partial udpates from
			// all other nodes, merges them together,
			// computes final clusters, check convergence
			// and sends result back to other nodes

			for (uint32 w = 1; w < device->getCommSize(); ++w)
			{
				// Receive partial state
				Array<Cluster<DataT>> partialState(numClusters);
				device->receiveBuffer(&partialState[0], numClusters, w, epoch);

				// Merge with master state
				for (uint32 k = 0; k < numClusters; ++k)
					clusters[k].fuse(partialState[k]);
			}

			// Check convergences
			/// @todo implement multiple methods
			float32 maxDrift = 0.f;
			for (uint32 k = 0; k < numClusters; ++k)
			{
				auto & cluster = clusters[k];

				// First, commit state
				cluster.commit();

				// Calculate cluster centroid drift w.r.t. to previous state
				const float32 drift = cluster.getDrift(prevState[k]);
				maxDrift = drift > maxDrift ? drift : maxDrift;
			}

			/// @todo Place this definition somewhere else. Threshold should be proportional to data, not fixed
			#define DRIFT_THRESHOLD 0.06f
			bFinalize = maxDrift < DRIFT_THRESHOLD;

			// Broadcast result to slave nodes
			for (uint32 w = 1; w < device->getCommSize(); ++w)
			{
				// Send convergence flag and master state
				device->send(bFinalize, w, epoch);
				device->sendBuffer(&clusters[0], numClusters, w, epoch);
			}
		}
		else
		{
			// Send partial state to master node
			device->sendBuffer(&clusters[0], numClusters, 0, epoch);

			// Receive convergence and master state
			device->receive(bFinalize, 0, epoch);
			device->receiveBuffer(&clusters[0], numClusters, 0, epoch);
		}

		// Next epoch
		++epoch;
	}

	// Return final state
	return clusters;
}

#endif