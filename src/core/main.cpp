#include <stdio.h>
#include <mpi.h>
#include <time.h>

#include "utils.h"
#include "point.h"
#include "cluster.h"

typedef unsigned long long uint64;

int main(int argc, char ** argv)
{
	srand(clock());

	Utils::generateRandomInput(128);

	Array<point> points = Utils::parseInput();
	
	const uint8 k = 4;
	Cluster<point> clusters[k];
	
	// Pick k furthest points
	Array<point> furthest = Utils::getKFurthest(points, k);
	for (uint8 i = 0; i < k; ++i)
	{
		clusters[i].addWeight(furthest[i]);
		clusters[i].commit();
	}
	
	for (uint64 i = 0; i < 64; ++i)
	{
		for (const point & p : points)
		{
			float minDist = FLT_MAX;
			uint8 clusterIdx = -1;

			// Find closest cluster
			for (uint8 j = 0; j < k; ++j)
			{
				const float dist = clusters[j].getDistance(p);
				if (dist < minDist)
				{
					minDist = dist;
					clusterIdx = j;
				}
			}

			// Add weight to closest cluster
			clusters[clusterIdx].addWeight(p);
		}

		// Commit changes
		for (uint8 j = 0; j < k; ++j)
			clusters[j].commit();
	}

	// Create groups
	Array<point> groups[k];
	for (const point & p : points)
	{
		float minDist = FLT_MAX;
		uint8 clusterIdx = -1;

		// Find closest cluster
		for (uint8 j = 0; j < k; ++j)
		{
			const float dist = clusters[j].getDistance(p);
			if (dist < minDist)
			{
				minDist = dist;
				clusterIdx = j;
			}
		}

		// Add point to closest group
		groups[clusterIdx].push_back(p);
	}

	for (uint8 j = 0; j < k; ++j)
	{
		printf("# Group %u\n", j);
		for (const point & p : groups[j]) p.print();
		printf("-----------------\n");
	}

	return 0;
}