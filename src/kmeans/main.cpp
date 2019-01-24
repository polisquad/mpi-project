#include "core_types.h"
#include "parallel/mpi_globals.h"
#include "parallel/mpi_device.h"
#include "containers/aligned_array.h"
#include "alg/data.h"
#include "alg/kmeans.h"
#include "alg/vector.h"

#include <vector>

int main(int32 argc, char ** argv)
{
	srand(clock());

	return 0;

	uint32 numDataPoints = 512 * 1024;
	uint32 numClusters = 24;

	std::vector<Vector<float32, 3>> dataPoints;
	dataPoints.reserve(numDataPoints);
	
#if 0
	for (uint32 i = 0; i < numDataPoints; ++i)
		dataPoints.push_back(Point2<float32>(
			rand() / (float64)RAND_MAX * 10.f,
			rand() / (float64)RAND_MAX * 10.f
		));
#elif 1
	for (uint32 i = 0; i < numDataPoints; ++i)
		dataPoints.push_back(Vector<float32, 3>(
			3,
			rand() / (float64)RAND_MAX * 10.f
		));
#elif 0
	// Read from file
	FILE * fin = fopen("data/target.csv", "r");
	char line[256];

	while (fgets(line, 256, fin))
	{
		Point2<float32> dataPoint;
		sscanf(line, "%f,%f", dataPoint.buffer, dataPoint.buffer + 1);

		dataPoints.push_back(dataPoint);
	}
#elif 0
	// Read from file
	FILE * fin = fopen("data/cars.csv", "r");
	char line[256];

	while (fgets(line, 256, fin))
	{
		Point2<float32> dataPoint;
		sscanf(line, "%f,%*f,%f", dataPoint.buffer, dataPoint.buffer + 1);

		dataPoints.push_back(dataPoint);
	}
#endif

	MPI::init();

	srand(clock());

	auto groups = clusterize(dataPoints, numClusters);

	MPI::shutdown();

#if 0
	FILE * fp = fopen("data/out.csv", "w");
	for (uint32 k = 0; k < numClusters; ++k)
	{
		const auto & group = groups[k];

		for (uint32 i = 0; i < group.size(); ++i)
		{
			const auto & dataPoint = group[i];
			fprintf(fp, "%.5f,%.5f,%u\n", dataPoint[0], dataPoint[1], k);
		}
	}

	fclose(fp);
#endif

	return 0;
}