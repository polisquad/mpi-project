#include <stdio.h>

#include "async/mpi.h"
#include "async/omp.h"
#include "containers/array.h"
#include "utils.h"
#include "cluster.h"
#include "point.h"
#include "kmeans.h"

#define MAX_ITERATIONS 100

int main(int argc, char ** argv)
{
    srand(clock());

    // Generate dummy dataset
    const uint64 datasetSize = 1024 * 4;
    Array<point> dataset; dataset.reserve(datasetSize);
    for (uint64 i = 0; i < datasetSize; ++i)
    {
        point p(6, 6);
        do
            p = point(rand() * 10.f / float32(RAND_MAX), rand() * 10.f / float32(RAND_MAX));
        while (sinf(p.y) * sinf(p.y) + cosf(p.x) * cosf(p.y) < 0.2f | p.x * p.y < 9.f);
        dataset.push_back(p);
    }

    MPI::init(argc, argv);
    const auto device = MPI::WorldDevice::getPtr();
    printf("Machine %s running on node %d/%u ...\n", device->getName().c_str(), device->getRank(), device->getCommSize());

    const uint32 numClusters = 5;
    // -1, no iter limits, run till convergence
    auto clusters = KMeans::genClusters(dataset, numClusters, -1, KMeans::EClusterInitialization::Furthest);

    //////////////////////////////////////////////////
    // OLD CODE
    //////////////////////////////////////////////////

    //return 0;

    // Common initialization
    /* float64 counter = 0.0;

    const uint32 numClusters = 4;
    //const uint64 datasetSize = 1024 * 4;

    // Generate dummy dataset
    //Array<point> dataset; dataset.reserve(datasetSize);
    for (uint64 i = 0; i < datasetSize; ++i)
    {
        point p(6, 6);
        do
            p = point(rand() * 10.f / float32(RAND_MAX), rand() * 10.f / float32(RAND_MAX));
        while (sinf(p.y) * sinf(p.y) + cosf(p.x) * cosf(p.y) < 0.2f | p.x * p.y < 9.f);
        dataset.push_back(p);
    }

    //////////////////////////////////////////////////
    // K-Means algorithm                            //
    //////////////////////////////////////////////////

    // Instead try random
    for (uint32 k = 0; k < numClusters; ++k)
        centroids.push_back(dataset[rand() % datasetSize]);

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

    // Check if converges
    bool bConv = false;

    // Start counter, for performance measure
    counter -= MPI_Wtime();

    // Epoch iterations
    for (uint32 epoch = 0; epoch < MAX_ITERATIONS & !bConv; ++epoch)
    {
        // Create a copy of the current state of the clusters
        // We use this copy to check convergence
        Array<Cluster<point>> prevClusters(clusters);

        #pragma omp parallel
        {
            // We use a private local copy on each thread
            // to avoid locking the resource for each udpate.
            // We rather delay the update on the shared clusters
            // till all threads have calculated the partial updates.
            Array<Cluster<point>> localClusters(clusters);

            #pragma omp for
            for (uint64 i = offset; i < datasetSize; i += step)
            {
                const auto & p = dataset[i];

                // Find closest cluster
                float32 minDist = localClusters[0].getDistance(p);
                uint32 minIdx = 0;

                for (uint32 k = 1; k < numClusters; ++k)
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

            for (uint8 k = 0; k < numClusters; ++k)
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


        // All slave hosts (rank > 1) send their updates
        // to the master host (rank == 0).
        // The master host receives the udpates, merges
        // them and send back the final clusters.
        if (device->getRank() > 0)
        {
            // Send updates to master
            device->sendBuffer(&clusters[0], numClusters, 0, epoch);

            // First receives convergence flag and final clusters
            device->receive(bConv, 0, epoch);
            device->receiveBuffer(&clusters[0], numClusters, 0, epoch);
        }
        else
        {
            const int32 commSize = device->getCommSize();
            for (uint32 i = 1; i < commSize; ++i)
            {
                // Receive partial udpates from slaves
                Array<Cluster<point>> partialUpdates(clusters);
                device->receiveBuffer(&partialUpdates[0], numClusters, i, epoch);

                // Fuse clusters
                for (uint32 k = 0; k < numClusters; ++k)
                    clusters[k].fuse(partialUpdates[k]);
            }

            // Check convergence
            bConv = true;
            for (uint32 k = 0; k < numClusters; ++k)
            {
                clusters[k].commit();
                bConv &= prevClusters[k] == clusters[k];
            }

            // Send convergence to all machines
            for (uint32 i = 1; i < commSize; ++i)
            {
                device->send(&bConv, i, epoch);
                device->sendBuffer(&clusters[0], numClusters, i, epoch);
            }
        }

        if (UNLIKELY(bConv == true)) printf("algorithm converges after %u epochs\n", epoch);
    }

    counter += MPI_Wtime(); */

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

    //printf("elapsed: %f s\n", counter);
    return 0;
}