#ifndef MPI_PROJECT_NODE_HPP
#define MPI_PROJECT_NODE_HPP

#include <core_types.h>
#include <mpi.h>
#include <vector>
#include <point.hpp>
#include <iterator>
#include <utils.hpp>
#include <algorithm>
#include <omp.h>
#include <chrono>

// Reduce(sum) vectors of points
#pragma omp declare reduction(reducePointVectors : std::vector<Point<float32>> : \
                              std::transform(omp_out.begin(), omp_out.end(), \
                                             omp_in.begin(), omp_out.begin(), \
                                             [](Point<float32> p1, Point<float32> p2){ \
                                                    return p1 + p2;})) \
                    initializer(omp_priv = omp_orig)

// Reduce(sum) vectors of int32
#pragma omp declare reduction(reduceIntVectors : std::vector<int32> : \
                              std::transform(omp_out.begin(), omp_out.end(), \
                                             omp_in.begin(), omp_out.begin(), \
                                             std::plus<int32>())) \
                    initializer(omp_priv = omp_orig)

// TODO
// -> separate interface
class Node {
private:
    int32 rank;
    int32 commSize;
    uint32 k;
    bool converged;
    float32 tolerance;
    bool verbose;

    // Loss <-- distortion measure
    float32 localLoss = 0.0;
    float32 loss = 0.0;

    std::vector<Point<float32>> dataset;
    uint64 dataPointSize;
    std::vector<Point<float32>> points;

    std::vector<Point<float32>> centroids;
    std::vector<Point<float32>> localCentroids;

    std::vector<uint64> localMemberships;
    std::vector<uint64> memberships;

    int32 receiveCount;
    std::vector<int32> displacements;
    std::vector<int32> sendCounts;

public:
    Node() = delete;

    explicit Node(uint32 k, float32 tol=1e-4, bool verbose=false) :
        dataPointSize(sizeof(Point<float32>)), converged(false),
        k(k), centroids(k), localCentroids(k), tolerance(tol), verbose(verbose)
    {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &commSize);
    }

    void loadPoints() {
        if (rank == 0) {
            dataset = loadDataset();
            sendCounts = getDataSplits(dataset.size(), commSize);
            for (int32 &sendCount : sendCounts) sendCount *= dataPointSize;
        }

        // Tell each node how many bytes it will receive in the next Scatterv operation
        MPI_Scatter(sendCounts.data(), 1, MPI_INT, &receiveCount, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Prepare Scatterv
        if (rank == 0) {
            displacements = std::vector<int32>(static_cast<uint64>(commSize));
            displacements[0] = 0;
            for (uint64 i = 1; i < commSize; i++) {
                displacements[i] = displacements[i - 1] + sendCounts[i - 1];
            }
        }
        points = std::vector<Point<float32>>(static_cast<uint64>((receiveCount / dataPointSize)));

        // Scatter points among nodes
        MPI_Scatterv(dataset.data(), sendCounts.data(), displacements.data(), MPI_BYTE,
                     points.data(), static_cast<int32>(receiveCount * dataPointSize), MPI_BYTE, 0, MPI_COMM_WORLD);

        localMemberships = std::vector<uint64>(points.size(), 0);
    }


    void selectRandomCentroids() {
        if (rank == 0) {
            uint64 numPoints = points.size();
            for (auto &centroid : centroids) {
                centroid = points[randomNextInt(numPoints)];
            }
        }
    }

    void receiveGlobalCentroids() {
        MPI_Bcast(centroids.data(), static_cast<int32>(k * dataPointSize), MPI_BYTE, 0, MPI_COMM_WORLD);
    }

    void receiveGlobal(uint32 epoch) {
        float32 oldLoss = loss;
        uint64 cluster;
        float32 newLocalLoss = 0;

        receiveGlobalCentroids();

        // Compute local loss
        #pragma omp parallel for schedule(static) private(cluster) \
            reduction(+: newLocalLoss)
        for (uint64 pIndex = 0; pIndex < points.size(); pIndex++) {
            cluster = localMemberships[pIndex];
            newLocalLoss += points[pIndex].getSquaredDistance(centroids[cluster]);
        }
        localLoss = newLocalLoss;

        // Gather local losses and compute overall loss
        MPI_Reduce(&localLoss, &loss, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

        // Broadcast overall loss
        MPI_Bcast(&loss, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

        converged = checkConvergence(oldLoss, loss);

        if (verbose && rank == 0) {
            printf("Completed epoch %u. Loss: %f\n", epoch, loss);
            if (converged) {
                printf("K-means algorithm took %u epochs to converge\n", epoch);
            }
        }

    }

    void optimize() {
        float32 minDist;
        float32 dist;
        uint64 cluster;

        std::vector<Point<float32>> newLocalCentroids(k, {0, 0});
        std::vector<int32> numPointsPerCentroid(k, 0);

        // TODO dynamic with different chunk sizes vs static
        #pragma omp parallel for schedule(static) private(minDist, dist, cluster) \
                reduction(reducePointVectors : newLocalCentroids) \
                reduction(reduceIntVectors : numPointsPerCentroid)
        for (uint64 pIndex = 0; pIndex < points.size(); pIndex++) {
            const Point<float32>& p = points[pIndex];
            minDist = p.getDistance(centroids[0]);
            cluster = 0;

            for (uint64 cIndex = 1; cIndex < k; cIndex++) {
                dist = p.getDistance(centroids[cIndex]);
                if (dist < minDist) {
                    minDist = dist;
                    cluster = cIndex;
                }
            }
            localMemberships[pIndex] = cluster;
            newLocalCentroids[cluster] += points[pIndex];
            numPointsPerCentroid[cluster] += 1;
        }

        // using a parallel for here is not worth unless there is a high number of clusters,
        // maybe add it with a if(k>x) clause?
        // #pragma omp parallel for schedule(static)
        for (uint64 cIndex = 0; cIndex < k; cIndex++) {
            if (numPointsPerCentroid[cIndex] != 0) {
                newLocalCentroids[cIndex] = newLocalCentroids[cIndex] / numPointsPerCentroid[cIndex];
            }
        }

        localCentroids = std::vector<Point<float32>>(newLocalCentroids);
    }


    void updateGlobal(uint32 epoch) {
        std::vector<Point<float32>> gatherLocalCentroids;
        int32 count = static_cast<int32>(k * dataPointSize);

        if (rank == 0) {
            gatherLocalCentroids = std::vector<Point<float32>>(commSize * k);
        }

        // Gather local centroids
        MPI_Gather(localCentroids.data(), count, MPI_BYTE,
                   gatherLocalCentroids.data(), count, MPI_BYTE, 0, MPI_COMM_WORLD);


        if (rank == 0) {
            std::vector<Point<float32>> newCentroids(k, {0, 0});

            // #pragma omp parallel for schedule(static) \
            reduction(reducePointVectors : newCentroids)
            for (uint64 i = 0; i < gatherLocalCentroids.size(); i++) {
                newCentroids[i % k] += gatherLocalCentroids[i];
            }

            // #pragma omp parallel for schedule(static)
            for (uint64 i = 0; i < k; i++) {
                newCentroids[i] /= commSize;
            }

            centroids = newCentroids;
        }
    }

    void finalize() {
        if (rank == 0) {
            memberships = std::vector<uint64>(dataset.size(), 0);
            for (int &displacement : displacements) {
                displacement /= dataPointSize;
            }
        }

        // Gather local memberships [Optional: root could aswell calculate all the memberships]
        MPI_Gatherv(localMemberships.data(), static_cast<int32>(receiveCount / dataPointSize), MPI_UNSIGNED_LONG_LONG,
                    memberships.data(), sendCounts.data(), displacements.data(), MPI_UNSIGNED_LONG_LONG,
                    0, MPI_COMM_WORLD);
    }

    void writeResults() const {
        if (rank == 0) {
            writeResultsToFile(memberships);
        }
    }

    bool checkConvergence(float32 oldLoss, float32 newLoss) const {
        return std::fabs(oldLoss - newLoss) <= tolerance;
    }

    bool hasConverged() const {
        return converged;
    }
};

#endif //MPI_PROJECT_NODE_HPP
