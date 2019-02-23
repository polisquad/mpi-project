#include <stdlib.h>
#include <stdio.h>

#include "containers/point.h"
#include "containers/cluster.h"
#include "mpi/node.h"
#include "utils/command_line.h"
#include "utils/data_generator.h"
#include "containers/array.h"

/// Print help dialog
void help();

int main(int32 argc, char ** argv)
{
	/* Array<point> values(1024, point());
	values[0][0] = 1.f;

	MPI::init(&argc, &argv);

	MPI_Datatype vecType = point::createMpiType();

#if 0
	int32 blocks[] = {8};
	MPI_Aint strides[] = {0};
	MPI_Datatype types[] = {MPI_FLOAT};
	MPI_Type_create_struct(1, blocks, strides, types, &vecType);
	MPI_Type_commit(&vecType);
#endif

	std::vector<int32> dataChunks{1024};
	std::vector<int32> displacements{0};
	Array<point> recvd(1024);
	recvd(1023);

	MPI_Scatterv(
		values.data(), dataChunks.data(), displacements.data(), vecType,
		recvd.data(), 1024, vecType,
		0, MPI_COMM_WORLD
	);

	values[0].print();
	recvd[0].print();	

	MPI::shutdown();

	return 0; */

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
		Array<int32> memberships;

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