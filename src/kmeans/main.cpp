#include "core_types.h"
#include "parallel/mpi_globals.h"
#include "parallel/mpi_device.h"
#include "alg/data.h"
#include "alg/kmeans.h"

#include <vector>

int main(int32 argc, char ** argv)
{
	srand(clock());

	const uint32 numDataPoints = 512 * 1024;
	std::vector<Point2<float32>> dataPoints;
	dataPoints.reserve(numDataPoints);
	
	for (uint32 i = 0; i < numDataPoints; ++i)
		dataPoints.push_back(Point2<float32>(
			rand() / (float64)RAND_MAX,
			rand() / (float64)RAND_MAX
		));	

	MPI::init();

	srand(clock());

	double start = MPI::time();
	auto groups = clusterize(dataPoints, 24);
	printf("%f\n", MPI::time() - start);

	MPI::shutdown();

	#if 0
	FILE * fp = fopen("data/out.csv", "w");
	for (uint32 k = 0; k < 5; ++k)
	{
		const auto & group = groups[k];

		for (uint32 i = 0; i < group.size(); ++i)
		{
			const auto & dataPoint = group[i];
			fprintf(fp, "%.5f,%.5f,%u\n", dataPoint.x, dataPoint.y, k);
		}
	}

	fclose(fp);
	#endif

	return 0;
}