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
// -> set some fields and avoid calling .size() on vectors every time
// -> Loss?
class Node {
private:
    int32 rank;
    int32 commSize;
    uint64 k;
    bool converged;

    std::vector<Point<float32>> points;

    std::vector<Point<float32>> centroids;
    std::vector<Point<float32>> localCentroids;

    std::vector<int32> localMemberships;
    std::vector<int32> memberships;
    std::vector<Point<float32>> dataset;

    int32 receiveCount[1];
    std::vector<int32> displacements;
    std::vector<int32> sendCounts;

    // TODO
    float32 localLoss;
    float32 loss;

public:
    Node() = delete;

    explicit Node(uint64 k) : converged(false), k(k), centroids(k), localCentroids(k) {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &commSize);
    }

    void loadPoints() {
        if (rank == 0) {
            dataset = loadDataset();
            sendCounts = getDataSplits(dataset.size(), commSize);
        }

        // Tell each node how many bytes it will receive in the next Scatterv operation
        for (int &sendCount : sendCounts) sendCount *= sizeof(Point<float32>);
        MPI_Scatter(sendCounts.data(), 1, MPI_INT, receiveCount, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Prepare Scatterv
        if (rank == 0) {
            displacements = std::vector<int32>(static_cast<uint64>(commSize));
            displacements[0] = 0;
            for (int32 i = 1; i < commSize; i++) {
                displacements[i] = displacements[i - 1] + sendCounts[i - 1];
            }
        }
        points = std::vector<Point<float32>>(static_cast<uint64>((receiveCount[0] / sizeof(Point<float32>))));

        // Scatter points among nodes
        MPI_Scatterv(dataset.data(), sendCounts.data(), displacements.data(), MPI_BYTE,
                     points.data(), receiveCount[0] * sizeof(Point<float32>), MPI_BYTE, 0, MPI_COMM_WORLD);

        localMemberships = std::vector<int32>(points.size(), 0);
    }


    void selectRandomCentroids() {
        if (rank == 0) {
            for (auto &centroid : centroids) {
                centroid = points[randomNextInt(points.size())];
            }
        }
    }

    void receiveGlobalCentroids() {
        std::vector<Point<float32>> oldCentroids;

        if (rank != 0) {
            oldCentroids = std::vector<Point<float32>>(centroids);
        }

        MPI_Bcast(centroids.data(), static_cast<int>(centroids.size() * sizeof(Point<float32>)),
                  MPI_BYTE, 0, MPI_COMM_WORLD);

        if (rank != 0) {
            converged = checkConvergence(oldCentroids, centroids);
        }
    }

    void optimizeMemberships() {
        float32 minDist;
        float32 dist;
        int32 cluster;

        // TODO dynamic[points / numthreads] vs static
        #pragma omp parallel for schedule(static) private(minDist, dist, cluster)
        for (int32 pIndex = 0; pIndex < points.size(); pIndex++) {
            Point<float32> p = points[pIndex];
            minDist = p.getDistance(centroids[0]);
            cluster = 0;

            for (int32 cIndex = 1; cIndex < centroids.size(); cIndex++) {
                dist = p.getDistance(centroids[cIndex]);
                if (dist < minDist) {
                    minDist = dist;
                    cluster = cIndex;
                }
            }
            localMemberships[pIndex] = cluster;
        }
    }

    void optimizeLocalCentroids() {
        std::vector<Point<float32>> newLocalCentroids(localCentroids.size(), {0, 0});
        std::vector<int32> numPointsPerCentroid(localCentroids.size(), 0);
        int32 cluster;

//         //TODO benchmark
//        #pragma omp parallel for schedule(static) private(cluster)
//        for (int32 pIndex = 0; pIndex < points.size(); pIndex++) {
//            cluster = localMemberships[pIndex];
//
//            #pragma omp critical
//            {
//                newLocalCentroids[cluster] += points[pIndex];
//                numPointsPerCentroid[cluster] += 1;
//            }
//        }

        // no lock overhead but reductions
        // TODO benchmark
        // TODO dynamic vs static
        #pragma omp parallel for schedule(static) private(cluster) \
                reduction(reducePointVectors : newLocalCentroids) \
                reduction(reduceIntVectors : numPointsPerCentroid)
        for (int32 pIndex = 0; pIndex < points.size(); pIndex++) {
            cluster = localMemberships[pIndex];
            newLocalCentroids[cluster] += points[pIndex];
            numPointsPerCentroid[cluster] += 1;
        }

        for (int cIndex = 0; cIndex < newLocalCentroids.size(); cIndex++) {
            if (numPointsPerCentroid[cIndex] != 0) {
                newLocalCentroids[cIndex] = newLocalCentroids[cIndex] / numPointsPerCentroid[cIndex];
            }
        }
        localCentroids = std::vector<Point<float32>>(newLocalCentroids);
    }

    void updateGlobalCentroids() {
        std::vector<Point<float32>> gatherLocalCentroids(commSize * centroids.size());
        int32 count = static_cast<int32>(localCentroids.size() * sizeof(Point<float32>));

        // Gather local centroids
        MPI_Gather(localCentroids.data(), count, MPI_BYTE,
                   gatherLocalCentroids.data(), count, MPI_BYTE, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            std::vector<Point<float32>> newCentroids(centroids.size(), {0, 0});

            for (int32 i = 0; i < gatherLocalCentroids.size(); i++) {
                newCentroids[i % centroids.size()] += gatherLocalCentroids[i];
            }

            for (int32 i = 0; i < centroids.size(); i++) {
                newCentroids[i] /= commSize;
            }
            converged = checkConvergence(centroids, newCentroids);
            centroids = newCentroids;
        }
    }

    void finalize() {
        if (rank == 0) {
            memberships = std::vector<int32>(dataset.size(), 0);
        }

        for (int &displacement : displacements) {
            displacement /= sizeof(Point<float32>);
        }

        // Gather local memberships [Optional: root could aswell calculate all the memberships]
        MPI_Gatherv(localMemberships.data(), receiveCount[0] / sizeof(Point<float32>) , MPI_INT,
                    memberships.data(), sendCounts.data(), displacements.data(), MPI_INT, 0, MPI_COMM_WORLD);
    }

    void writeResults() const {
        if (rank == 0) {
            writeResultsToFile(memberships);
        }
    }

    bool checkConvergence(const std::vector<Point<float32>>& oldCentroids,
                          const std::vector<Point<float32>>& newCentroids) const {
        for (int32 i = 0; i < oldCentroids.size(); i++) {
            if (newCentroids[i] != oldCentroids[i]) {
                return false;
            }
        }
        return true;
    }

    bool hasConverged() const {
        return converged;
    }
};

#endif //MPI_PROJECT_NODE_HPP
