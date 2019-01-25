#include "core_types.h"
#include "utils/command_line.h"
#include "utils/csv_parser.h"
#include "utils/csv_writer.h"
#include "parallel/mpi_globals.h"
#include "parallel/mpi_device.h"
#include "containers/aligned_array.h"
#include "alg/data.h"
#include "alg/kmeans.h"
#include "alg/vector.h"

CommandLine * gCommandLine;

int main(int32 argc, char ** argv)
{
	srand(clock());

	// Parse command line
	gCommandLine = new CommandLine(argc, argv);
	
#if 0
	#if 0
		// Data points vector
		Array<Point2<float32>> dataPoints;

		uint32 numDataPoints = 512 * 1024;
		uint32 numClusters = 24;

		for (uint32 i = 0; i < numDataPoints; ++i)
			dataPoints.push(Point2<float32>(
				rand() / (float64)RAND_MAX * 10.f,
				rand() / (float64)RAND_MAX * 10.f
			));
		// Data points vector
		Array<Vector<float32, 2>> dataPoints;
	#elif 1	
		uint32 numDataPoints = 512 * 1024;
		uint32 numClusters = 24;
		
		for (uint32 i = 0; i < numDataPoints; ++i)
			dataPoints.push(Vector<float32, 2>(
				8,
				rand() / (float64)RAND_MAX * 10.f
			));
	#endif
#endif

	// Read from file
	std::string inputFilename, outputFilename;
	if (CommandLine::get().getValue("input", inputFilename))
	{
		// Parse input
		CsvParser<float32> parser(inputFilename);
		auto dataPoints = parser.parse(1); // Pop header

		// Get number of clusters
		uint32 numClusters = 2;
		CommandLine::get().getValue("--clusters", numClusters);

		MPI::init();

		srand(clock());

		auto groups = clusterize(dataPoints, numClusters);

		MPI::shutdown();

		if (CommandLine::get().getValue("output", outputFilename))
		{
			CsvWriter<float32> writer(outputFilename);
			writer.write(groups);
		}
	}
	else
		fprintf(stderr, "No input file specified\n");

	return 0;
}