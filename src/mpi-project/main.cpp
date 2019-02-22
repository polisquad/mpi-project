#include <stdlib.h>
#include <stdio.h>

#include "containers/point.h"
#include "containers/cluster.h"
#include "mpi/node.h"
#include "utils/command_line.h"
#include "utils/data_generator.h"

int main(int32 argc, char ** argv)
{
	// Init random
	srand(clock());

	MPI::init(&argc, &argv);

	// Create global command line
	CommandLine gCommandLine(argc, argv);

	// Start node
	{
		// MPI local node
		Node<float32> node;

		// Read or create dataset
		{
			std::string filename;
			if (CommandLine::get().getValue("input", filename))
				node.readDataset(filename);
			else
				node.createDataset();
		}

		// Run algorithm
		{
			MPI::ScopedTimer _;
			node.run();
		}

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