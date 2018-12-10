#include <stdio.h>

#include "async/mpi.h"
#include "async/omp.h"
#include "containers/array.h"
#include "utils.h"
#include "cluster.h"
#include "point.h"

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
	srand(clock());

	// Common initialization
	float64 counter = 0.0;
	const uint32 numClusters = 6;
	const uint64 datasetSize = 256;
	Array<point> dataset; dataset.reserve(datasetSize);

	// Generate dummy dataset
	for (uint64 i = 0; i < datasetSize; ++i)
		dataset.push_back(point(rand() * 10.f / float32(RAND_MAX), rand() * 10.f / float32(RAND_MAX)));

	// Get furthest centroids
	Array<point> centroids = Utils::getKFurthest(dataset, numClusters);
	/* Array<point> centroids;
	for (uint32 k = 0; k < numClusters; ++k)
		centroids.push_back(dataset[rand() % datasetSize]); */
	ASSERT(centroids.size() == numClusters, "Number of centroids doesn't match number of clusters");

	// Create clusters
	Array<Cluster<point>> clusters;
	for (uint32 k = 0; k < numClusters; ++k)
	{
		clusters.emplace_back();
		clusters[k].addWeight(centroids[k]);
		clusters[k].commit();
	}

	MPI::init(argc, argv);
	srand(clock());

	// World device
	MPI::WorldDeviceRef device = MPI::WorldDevice::getPtr();

	printf("Machine %s running on node %d/%u ...\n", device->getName().c_str(), device->getRank(), device->getCommSize());

	const uint64 offset = device->getRank();
	const uint32 step = device->getCommSize();

	counter -= MPI_Wtime();

	for (uint32 epoch = 0; epoch < 256; ++epoch)
	{
		#pragma omp parallel for schedule(static)
		for (uint64 i = offset; i < datasetSize; i += step)
		{
			const auto & p = dataset[i];

			// Find closest cluster
			float32 minDist = clusters[0].getDistance(p);
			uint32 minIdx = 0;

			for (uint32 k = 1; k < numClusters; ++k)
			{
				float32 dist = clusters[k].getDistance(p);

				if (dist < minDist)
				{
					minDist = dist;
					minIdx = k;
				}
			}
			
			clusters[minIdx].addWeight(p);
		}

		if (device->getRank() > 0)
		{
			// Send clusters to master
			for (uint32 k = 0; k < numClusters; ++k)
				device->send(&clusters[k], 0, k);
			
			// Receive udpated clusters
			for (uint32 k = 0; k < numClusters; ++k)
			{
				device->receive(&clusters[k], 0, k);

				// Commit changes
				clusters[k].commit();
				//printf("(%d, %u): ", device->getRank(), k); clusters[k].printDebug();
			}
		}
		else
		{
			for (uint32 i = 1; i < device->getCommSize(); ++i)
			{
				for (uint32 k = 0; k < numClusters; ++k)
				{
					// Receive cluster udpate
					Cluster<point> cluster;
					device->receive(&cluster, i, k);

					// Fuse cluster
					clusters[k] += cluster;
				}
			}

			// Send back udpated clusters
			for (uint32 i = 1; i < device->getCommSize(); ++i)
			{
				for (uint32 k = 0; k < numClusters; ++k)
				{
					device->send(&clusters[k], i, k);
				}
			}
		}
	}

	counter += MPI_Wtime();

	MPI::shutdown();

#if 1
	FILE * fp = fopen("./data/out.csv", "w");
	for (const auto & p : dataset)
	{
		// Find closest cluster
		float32 minDist = clusters[0].getDistance(p);
		uint32 minIdx = 0;

		for (uint32 k = 1; k < numClusters; ++k)
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