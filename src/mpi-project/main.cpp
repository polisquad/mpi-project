#include <stdlib.h>
#include <stdio.h>

#include "containers/point.h"
#include "containers/cluster.h"
#include "mpi/node.h"
#include "utils/command_line.h"
#include "utils/data_generator.h"

/// Print help dialog
void help();

int main(int32 argc, char ** argv)
{
	// Init random
	srand(clock());

	// Create global command line
	CommandLine gCommandLine(argc, argv);

	if (gCommandLine.getValue("help"))
	{
		help();
		return 0;
	}

	MPI::init(&argc, &argv);

	// Start node
	{
		// MPI local node
		Node<float32> node;

		// Memberships vector
		std::vector<int32> memberships;

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
			node.run(memberships);
		}

		// Write output file
		{
			std::string filename;
			if (CommandLine::get().getValue("output", filename))
				node.writeDataset(filename);
		}
	}

	MPI::shutdown();

	return 0;
}

void help()
{
	printf("\n");
	printf("Usage: mpi-project [input] [output] [options]\n");
	printf("\n");
	printf("Options:\n");
	printf("  --output {filename}       overrides output file\n");
	printf("  --num-clusters {num}      sets number of clusters (default: 5)\n");
	printf("  --init-method {method}    cluster initialization method ('random' or 'furthest', default: 'random')\n");
	printf("  --num-epochs {num}        maximum number of epochs to simulate (default: 100)\n");
	printf("  --gen-num {num}           if no input is specified, number of data points to generate (default: 1024)\n");
	printf("  --gen-dim {num}           if no input is specified, dimension of the generated data (default: 2)\n");
	printf("\n");
}