
#include <node.hpp>

int main(int argc, char **argv) {
    // TODO
    // -> generalize types(both float32 and float64)
    // -> generalize to n-dimensional data point
    // -> benchmark in LAN with and without -03

    MPI_Init(&argc, &argv);

    // TODO
    // these are read from command line args
    uint64 k = 5;
    int32 maxNumEpochs = 500;
    float32 tol = 1e-4;
    bool verbose = true;

    Node thisNode(k, tol, verbose);

    // Generate dummy dataset
    writeDatasetToFile(generateDummyDataset());

    // Assign equally data points to each machine
    thisNode.loadPoints();

    // Set initial centroids
    thisNode.selectRandomCentroids();

    int32 epoch = 0;

    for (epoch = 0; epoch < maxNumEpochs; epoch++) {

        // Receive current centroids and loss
        thisNode.receiveGlobal(epoch);

        // Check convergence
        if (thisNode.hasConverged()) {
            break;
        }

        // Compute memberships of each point
        thisNode.optimizeMemberships();

        // Compute local centroids and local loss according to local membership view
        thisNode.optimizeLocalCentroids();

        // Compute new centroids and new loss
        thisNode.updateGlobal(epoch);
    }

    thisNode.finalize();
    thisNode.writeResults();

    MPI_Finalize();
    return 0;
}