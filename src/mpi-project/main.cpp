#include <stdlib.h>
#include <stdio.h>

#include "containers/point.h"
#include "containers/cluster.h"
#include "mpi/node.h"
#include "utils/command_line.h"

int main(int32 argc, char ** argv)
{
	MPI::init(&argc, &argv);

	{
		// MPI local node
		Node<float32> node;

		// Read dataset
		{
			std::string filename;
			if (!CommandLine::get().getValue("input", filename))
			{
				// An input is required, otherwise execution fails
				fprintf(stderr, "Usage: mpi-project input [output] [options]\n");
				return 1;
			}

			node.readDataset(filename);
		}

		// Run algorithm
		node.run();

		// Write output file
		{
			std::string filename;
			if (CommandLine::get().getValue("output", filename));
				node.writeDataset("data/out.csv");
		}
	}

	MPI::shutdown();

	return 0;
}