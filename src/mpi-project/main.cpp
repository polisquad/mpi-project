#include <stdlib.h>
#include <stdio.h>

#include "containers/point.h"
#include "containers/cluster.h"
#include "mpi/node.h"

int main(int32 argc, char ** argv)
{
	MPI::init(&argc, &argv);

	{
		// MPI local node
		Node<float32> node;

		// Read dataset
		node.readDataset("data/in.csv");

		// Run algorithm
		node.run();

		// Write output file
		node.writeDataset("data/out.csv");
	}

	MPI::shutdown();

	return 0;
}