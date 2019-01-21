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
    // these are read from command line args
    uint64 k = 5;
    int32 maxNumEpochs = 10000;

    Node thisNode(k);

    // Generate dummy dataset
    writeDatasetToFile(generateDummyDataset());

    // Assign equally data points to each machine
    thisNode.loadPoints();

    // Set initial centroids
    thisNode.selectRandomCentroids();

    bool converged = false;
    int32 epoch = 0;

    for (epoch = 0; epoch < maxNumEpochs; epoch++) {

        // Broadcast current centroids to all machines
        thisNode.receiveGlobalCentroids();

        // Check convergence
        // TODO maybe it's better to check the loss, for convergence, rather than every single centroid
        if (thisNode.hasConverged()) {
            break;
        }

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

    printf("K-means algorithm took %d epochs to converge\n", epoch);
}