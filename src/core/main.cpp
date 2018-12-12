#include <stdio.h>

#include "async/mpi.h"
#include "async/omp.h"
#include "containers/array.h"
#include "utils.h"
#include "cluster.h"
#include "point.h"

#define MAX_ITERATIONS 100
#define NUM_CLUSTERS 4
#define DATASET_SIZE 1024 * 4

Array<Cluster<Point<float>>> initClusters(const Array<Point<float>> &dataset);

void updateClusters(const Array<Point<float>> &dataset, Array<Cluster<Point<float>>> &clusters);

bool mergeUpdates(Array<Cluster<Point<float>>> &clusters, uint32 epoch);

int main(int argc, char ** argv)
{
	srand(clock());

	// Common initialization
	float64 counter = 0.0;
    Array<Point<float>> dataset = generateDataset(DATASET_SIZE);

    //////////////////////////////////////////////////
	// K-Means algorithm                            //
	//////////////////////////////////////////////////
	Array<Cluster<Point<float>>> clusters = initClusters(dataset);

	MPI::init(argc, argv);

	// Check if converges
	bool bConv = false;

	// Start counter, for performance measure
	counter -= MPI_Wtime();

	// Epoch iterations
	uint32 epoch;
	for (epoch = 0; epoch < MAX_ITERATIONS & !bConv; ++epoch)
	{
		updateClusters(dataset, clusters);
		bConv = mergeUpdates(clusters, epoch);
	}
	counter += MPI_Wtime();
	printf("algorithm converges after %u epochs\n", epoch);
	MPI::shutdown();

#if 1
	FILE * fp = fopen("./data/out.csv", "w");
	for (const auto & p : dataset)
	{
		// Find closest cluster
		float32 minDist = clusters[0].getDistance(p);
		uint32 minIdx = 0;

		for (uint32 k = 1; k < NUM_CLUSTERS; ++k)
		{
			float32 dist = clusters[k].getDistance(p);

			if (dist < minDist)
			{
				minDist = dist;
				minIdx = k;
			}
		}
		
		fprintf(fp, "%f,%f,%d\n", p.x, p.y, minIdx);
	}
#endif

	printf("elapsed: %f s\n", counter);
	return 0;
}

bool mergeUpdates(Array<Cluster<Point<float>>> &clusters, uint32 epoch) {

	/**
     * All slave hosts (rank > 1) send their updates
     * to the master host (rank == 0).
     * The master host receives the udpates, merges
     * them and send back the final clusters.
     */
	MPI::WorldDeviceRef device = MPI::WorldDevice::getPtr();
	bool bConv;
	Array<Cluster<Point<float>>> prevClusters(clusters);
	if (device->getRank() > 0)
	{
		// Send updates to master
		device->sendBuffer(&clusters[0], NUM_CLUSTERS, 0, epoch);

		// First receives convergence flag and final clusters
		device->receive(&bConv, 0, epoch);
		device->receiveBuffer(&clusters[0], NUM_CLUSTERS, 0, epoch);
	}
	else
	{
		const int32 commSize = device->getCommSize();
		for (uint32 i = 1; i < commSize; ++i)
		{
			// Receive partial udpates from slaves
			Array<Cluster<point>> partialUpdates(clusters);
			device->receiveBuffer(&partialUpdates[0], NUM_CLUSTERS, i, epoch);

			// Fuse clusters
			for (uint32 k = 0; k < NUM_CLUSTERS; ++k)
				clusters[k].fuse(partialUpdates[k]);
		}

		// Check convergence
		bConv = true;
		for (uint32 k = 0; k < NUM_CLUSTERS; ++k)
		{
			clusters[k].commit();
			bConv &= prevClusters[k] == clusters[k];
		}

		// Send convergence to all machines
		for (uint32 i = 1; i < commSize; ++i)
		{
			device->send(&bConv, i, epoch);
			device->sendBuffer(&clusters[0], NUM_CLUSTERS, i, epoch);
		}
	}
	return bConv;
}

void updateClusters(const Array<Point<float>> &dataset, Array<Cluster<Point<float>>> &clusters) {
	// Create a copy of the current state of the clusters
	// We use this copy to check convergence
	#pragma omp parallel
	{
		// We use a private local copy on each thread
		// to avoid locking the resource for each udpate.
		// We rather delay the update on the shared clusters
		// till all threads have calculated the partial updates.
		Array<Cluster<point>> localClusters(clusters);
		MPI::WorldDeviceRef device = MPI::WorldDevice::getPtr();
		const uint64 offset = device->getRank();
		const uint32 step = device->getCommSize();
		// World device
//		printf("Machine %s running on node %d/%u ...\n", device->getName().c_str(), device->getRank(),
//				device->getCommSize());

		#pragma omp for
		for (uint64 i = offset; i < DATASET_SIZE; i += step)
		{
			const auto & p = dataset[i];

			// Find closest cluster
			float32 minDist = localClusters[0].getDistance(p);
			uint32 minIdx = 0;

			for (uint32 k = 1; k < NUM_CLUSTERS; ++k)
			{
				const float32 dist = localClusters[k].getDistance(p);

				if (dist < minDist)
				{
					minDist = dist;
					minIdx = k;
				}
			}

			// Add weight to the closest cluster
			localClusters[minIdx].addWeight(p);
		}

		for (uint8 k = 0; k < NUM_CLUSTERS; ++k)
		{
			auto & cluster = clusters[k];

			// Fuse and lock the cluster resource.
			// We don't want separate updates to overlap.
			{
				OMP::ScopeLock _(&cluster.getUsageGuard());
				cluster.fuse(localClusters[k]);
			}
		}
	}
}

Array<Cluster<Point<float>>> initClusters(const Array<Point<float>> &dataset) {
	Array<point> centroids/*  = Utils::getKFurthest(dataset, NUM_CLUSTERS);
	ASSERT(centroids.size() == NUM_CLUSTERS, "Number of centroids doesn't match number of clusters") */;

	// Instead try random
	for (uint32 k = 0; k < NUM_CLUSTERS; ++k)
		centroids.push_back(dataset[rand() % DATASET_SIZE]);

	// Create clusters
	Array<Cluster<point>> clusters;
	for (uint32 k = 0; k < NUM_CLUSTERS; ++k)
	{
		clusters.emplace_back();
		clusters[k].addWeight(centroids[k]);
		clusters[k].commit();
	}
	return clusters;
}

