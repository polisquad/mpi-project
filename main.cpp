#include <vector>
#include <node.hpp>
#include <point.hpp>

int main(int argc, char **argv) {

    // TODO
    // -> generalize types(both float32 and float64)
    // -> generalize to n-dimensional data point
    // -> also compute loss at each epoch?
    // -> benchmark in LAN with and without -03
    MPI_Init(&argc, &argv);

    // Number of clusters
    // TODO
    // this is read from command line args
    uint64 k = 5;
    Node thisNode(k);

    // Generate dummy dataset
    writeDatasetToFile(generateDummyDataset());

    // Assign equally data points to each machine
    thisNode.loadPoints();

    // Set initial centroids
    thisNode.selectRandomCentroids();

    int numEpochs = 1000;

    // TODO while not converged
    for (int i = 0; i < numEpochs; i++) {

        // Broadcast current centroids to all machines
        thisNode.receiveGlobalCentroids();

        // Compute membership of each point
        thisNode.optimizeMemberships();

        // Compute local centroid(calculating local mean for each cluster) according to local membership view
        thisNode.optimizeLocalCentroids();

        // Compute global centroids(just sum the local means received from each machine for each cluster)
        thisNode.updateGlobalCentroids();
    }

    thisNode.finalize();
    thisNode.writeResults();

    MPI_Finalize();
}