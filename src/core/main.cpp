#include <stdio.h>

#include "async/mpi.h"
#include "async/omp.h"
#include "utils.h"
#include "cluster.h"
#include "data.h"

#define MAX_ITERATIONS 1000
#define NUM_CLUSTERS 3
#define DATASET_SIZE 1024 * 4

template<typename T>
Array<Cluster<T>> initClusters(const Array<T> &dataset);

template<typename T>
void updateClusters(const Array<T> &dataset, Array<Cluster<T>> &clusters);

template<typename T>
bool mergeUpdates(Array<Cluster<T>> &clusters, uint32 epoch);

template<typename T>
void writeResult(const Array<T> &dataset, const Array<Cluster<T>> &clusters);

void kMeans();

Array<Point<float>> generateDataset(uint64 datasetSize);

template<typename T>
uint32 findClosestCluster(const Array<Cluster<T>> &clusters, const T &p);

void printResults(const Array<Data> &dataset, Array<Cluster<Data>> &clusters);

float computePurity(const Array<Data> &dataset, Array<Cluster<Data>> &clusters);

int main(int argc, char **argv)
{
    MPI::init(argc, argv);
    kMeans();

    return 0;
}

void kMeans()
{
    srand(clock());
    float64 counter = 0.0;
//    Array<Point<float>> dataset = generateDataset(DATASET_SIZE);
//    Array<Cluster<Point<float>>> clusters = initClusters(dataset);
    vector<Data> dataset = Data::readCSVFileNormalized("dataset/iris.data");
    vector<Cluster<Data>> clusters = initClusters(dataset);

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
    printResults(dataset, clusters);

//#if 1
//    writeResult(dataset, clusters);
//#endif

    printf("elapsed: %f s\n", counter);
}

template<typename T>
Array<Cluster<T>> initClusters(const Array<T> &dataset)
{
    Array<T> centroids/*  = Utils::getKFurthest(dataset, NUM_CLUSTERS);
	ASSERT(centroids.size() == NUM_CLUSTERS, "Number of centroids doesn't match number of clusters") */;

    // Instead try random
    for (uint32 k = 0; k < NUM_CLUSTERS; ++k)
        centroids.push_back(dataset[rand() % dataset.size()]);

    // Create clusters
    Array<Cluster<T>> clusters;
    for (uint32 k = 0; k < NUM_CLUSTERS; ++k)
    {
        clusters.emplace_back();
        clusters[k].addWeight(centroids[k]);
        clusters[k].commit();
    }
    return clusters;
}

template<typename T>
void updateClusters(const Array<T> &dataset, Array<Cluster<T>> &clusters)
{
    // Create a copy of the current state of the clusters
    // We use this copy to check convergence
    #pragma omp parallel
    {
        // We use a private local copy on each thread
        // to avoid locking the resource for each udpate.
        // We rather delay the update on the shared clusters
        // till all threads have calculated the partial updates.
        Array<Cluster<T>> localClusters(clusters);
        MPI::WorldDeviceRef device = MPI::WorldDevice::getPtr();
        const uint64 offset = device->getRank();
        const uint32 step = device->getCommSize();
        // World device
//		printf("Machine %s running on node %d/%u ...\n", device->getName().c_str(), device->getRank(),
//				device->getCommSize());

        #pragma omp for
        for (uint64 i = offset; i < dataset.size(); i += step)
        {
            const auto &p = dataset[i];
            uint32 minIdx = findClosestCluster(localClusters, p);
            localClusters[minIdx].addWeight(p);
        }

        for (uint8 k = 0; k < NUM_CLUSTERS; ++k)
        {
            auto &cluster = clusters[k];

            // Fuse and lock the cluster resource.
            // We don't want separate updates to overlap.
            {
                OMP::ScopeLock _(&cluster.getUsageGuard());
                cluster.fuse(localClusters[k]);
            }
        }
    }
}

template<typename T>
bool mergeUpdates(Array<Cluster<T>> &clusters, uint32 epoch)
{

    /**
     * All slave hosts (rank > 1) send their updates
     * to the master host (rank == 0).
     * The master host receives the udpates, merges
     * them and send back the final clusters.
     */
    MPI::WorldDeviceRef device = MPI::WorldDevice::getPtr();
    bool bConv;
    Array<Cluster<T>> prevClusters(clusters);
    if (device->getRank() > 0)
    {
        // Send updates to master
        device->sendBuffer(&clusters[0], NUM_CLUSTERS, 0, epoch);

        // First receives convergence flag and final clusters
        device->receive(&bConv, 0, epoch);
        device->receiveBuffer(&clusters[0], NUM_CLUSTERS, 0, epoch);
    } else
    {
        const int32 commSize = device->getCommSize();
        for (uint32 i = 1; i < commSize; ++i)
        {
            // Receive partial udpates from slaves
            Array<Cluster<T>> partialUpdates(clusters);
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

template<typename T>
void writeResult(const Array<T> &dataset, const Array<Cluster<T>> &clusters)
{
    FILE *fp = fopen("./data/out.csv", "w");
    for (const auto &p : dataset)
    {
        uint32 minIdx = findClosestCluster(clusters, p);

        fprintf(fp, "%f,%f,%d\n", p.x, p.y, minIdx);
    }
}

/**
 * Generates a dummy dataset
 * @param datasetSize
 * @return
 */
Array<Point<float>> generateDataset(uint64 datasetSize)
{
    Array<point> dataset;
    dataset.reserve(datasetSize);
    for (uint64 i = 0; i < datasetSize; ++i)
    {
        point p(6, 6);
        do
            p = point(rand() * 10.f / float32(RAND_MAX), rand() * 10.f / float32(RAND_MAX));
        while (sinf(p.y) * sinf(p.y) + cosf(p.x) * cosf(p.y) < 0.2f | p.x * p.y < 9.f);
        dataset.push_back(p);
    }
    return dataset;
}

template<typename T>
uint32 findClosestCluster(const Array<Cluster<T>> &clusters, const T &p)
{// Find closest cluster
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
    return minIdx;
}

void printResults(const Array<Data> &dataset, Array<Cluster<Data>> &clusters)
{
    std::cout << "Purity: " << computePurity(dataset, clusters) << "\n";
}

/**
 * Computes the purity, which is the total number of data points defined correctly in the clusters.
 * @param dataset
 * @param clusters
 * @return
 */
float computePurity(const Array<Data> &dataset, Array<Cluster<Data>> &clusters)
{
    float purity = 0;
    unsigned long size = dataset.size();
    std::map<int, std::map<string, int>> assignments;
    for (auto const &p : dataset)
    {
        uint32 minIdx = findClosestCluster(clusters, p);
        assignments[minIdx][p.getCls()] += 1;
    }
    for (auto const &assignment : assignments)
    {
        string max_k;
        float max_v = 0;
        for (auto const &label : assignment.second)
        {
            if (label.second > max_v)
            {
                max_k = label.first;
                max_v = label.second;
            }
        }
        purity += max_v / size;
    }
    return purity;
}
