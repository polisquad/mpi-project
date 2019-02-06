#pragma once

#include "core_types.h"
#include "parallel/mpi.h"
#include "parallel/omp.h"
#include "data.h"
#include "vector.h"
#include "utils/command_line.h"
#include "utils/csv_parser.h"
#include "utils/csv_writer.h"
#include "utils/utils.h"
#include <vector>
#include <iterator>
#include <algorithm>

// Point data type
using Point = Vec<float32>;

// Reduce(sum) vectors of points
#pragma omp declare reduction(\
	reduceLocalCentroids :\
	std::vector<Point> :\
	std::transform(\
		omp_out.begin(), omp_out.end(),\
		omp_in.begin(), omp_out.begin(),\
		std::plus<Point>()\
	)\
)\
initializer(omp_priv = omp_orig)

// Reduce(sum) vectors of int32
#pragma omp declare reduction(\
	reduceIntVectors :\
	std::vector<int32> : \
	std::transform(\
		omp_out.begin(), omp_out.end(),\
		omp_in.begin(), omp_out.begin(),\
		std::plus<int32>()\
	)\
)\
initializer(omp_priv = omp_orig)

// TODO
// -> separate interface
class Node : public MPI::Device
{
protected:
	/// Num clusters
	uint32 k;

	/// Convergence flag
	bool converged;

	/// Convergence tolerance threshold
	float32 tolerance;

	/// Loss <-- distortion measure
	/// @{
	float32 localLoss = 0.0;
	float32 loss = 0.0;
	/// @}

	/// Dataset (root only)
	std::vector<Point> dataset;

	/// Subset of points allocated to this node
	std::vector<Point> points;

	/// Global clusters (root only)
	std::vector<Cluster<Point>> clusters;

	/// Local clusters of each node
	std::vector<Cluster<Point>> localCentroids;

	/// @todo unnecessary burden
	std::vector<uint32> localMemberships;
	std::vector<uint32> memberships;

	int32 receiveCount;
	std::vector<int32> displacements;
	std::vector<int32> sendCounts;

public:
	/// Initialization constructor
	explicit Node(uint32 _k, float32 _tolerance = 1e-4) :
		k(_k),
		localCentroids(k),
		converged(false),
		tolerance(_tolerance)
	{
		// Create mpi data types
		Point::				createMpiDataType();
		Cluster<Point>::	createMpiDataType();
	}

	void loadPoints() {
		
		if (id == 0)
		{
			// Import dataset from file
			std::string filename;
			if (CommandLine::get().getValue("input", filename))
			{
				CsvParser<float32> parser(filename);
				dataset = parser.parse(1);
			}
			else
			{
				fprintf(stderr, "Usage: kmeans <input> [<output>] [parameters*]");
				exit(1);
			}

			// Evenly splits dataset among nodes
			sendCounts = getDataSplits(dataset.size(), getCommSize());
		}

		// Tell each node how many points it will receive in the next Scatterv operation
		MPI_Scatter(sendCounts.data(), 1, MPI_INT, &receiveCount, 1, MPI_INT, 0, communicator);

		// Prepare Scatterv
		if (id == 0) {

			displacements = std::vector<int32>(getCommSize(), 0);

			for (uint64 i = 1; i < getCommSize(); i++)
				displacements[i] = displacements[i - 1] + sendCounts[i - 1];
		}

		// Allocate space for points to be received
		points = std::vector<Point>(receiveCount);

		// Scatter points among nodes
		auto pointType = Point::type;
		MPI_Scatterv(
			dataset.data(), sendCounts.data(), displacements.data(), pointType,
			points.data(), receiveCount, pointType, 0, communicator
		);

		// Initialize local memberships
		localMemberships = std::vector<uint32>(points.size(), 0);
	}

	void selectRandomCentroids() {

		if (id == 0)
		{
			std::string initMethod;
			CommandLine::get().getValue("--init", initMethod);

			if (initMethod == "furthest")
				clusters = Cluster<Point>::initFurthest(dataset, k);
			else
				clusters = Cluster<Point>::initRandom(dataset, k);
		}
	}

	void FORCE_INLINE receiveGlobalCentroids() {

		MPI_Bcast(clusters.data(), k, Cluster<Point>::type, 0, communicator);
	}

	void receiveGlobal(uint32 epoch) {

		// @todo do something with loss, you know ...
		float32
			oldLoss = loss,
			newLocalLoss = 0;

		receiveGlobalCentroids();

		// Compute local loss
		#pragma omp parallel for schedule(static) reduction(+ : newLocalLoss)
		for (uint64 pIndex = 0; pIndex < points.size(); pIndex++)
		{
			const uint32 cluster = localMemberships[pIndex];
			newLocalLoss += clusters[cluster].getDistance(points[pIndex]);
		}
		localLoss = newLocalLoss;

		// Gather local losses and compute overall loss
		MPI_Reduce(&localLoss, &loss, 1, MPI_FLOAT, MPI_SUM, 0, communicator);

		// Broadcast overall loss
		MPI_Bcast(&loss, 1, MPI_FLOAT, 0, communicator);

		converged = checkConvergence(oldLoss, loss);

		/* if (verbose && id == 0) {
			printf("Completed epoch %u. Loss: %f\n", epoch, loss);
			if (converged) {
				printf("K-means algorithm took %u epochs to converge\n", epoch);
			}
		} */
	}

	void optimize() {

		std::vector<Point> newLocalCentroids(k, Point());
		std::vector<int32> numVectorsPerCentroid(k, 0);

		// TODO dynamic with different chunk sizes vs static
		#pragma omp parallel for schedule(static)\
			reduction(reduceLocalCentroids : newLocalCentroids)\
			reduction(reduceIntVectors : numVectorsPerCentroid)
		for (uint64 pIndex = 0; pIndex < points.size(); pIndex++)
		{
			const Point& p = points[pIndex];

			float32 minDist	= clusters[0].getDistance(p);
			uint32 cluster	= 0;

			// Find closest cluster index
			for (uint32 cIndex = 1; cIndex < k; cIndex++)
			{
				const float32 dist = clusters[cIndex].getDistance(p);
				if (dist < minDist) minDist = dist, cluster = cIndex;
			}

			localMemberships[pIndex] = cluster;
			newLocalCentroids[cluster] += points[pIndex];
			numVectorsPerCentroid[cluster] += 1;
		}
		
		for (uint32 cIndex = 0; cIndex < k; cIndex++)
			clusters[cIndex].addWeight(newLocalCentroids[k], numVectorsPerCentroid[k]);
	}


	void updateGlobal(uint32 epoch) {
		std::vector<Cluster<Point>> remoteClusters;

		if (id == 0)
			remoteClusters = std::vector<Cluster<Point>>(getCommSize() * k);

		// Gather clusters from slave nodes
		auto clusterType = Cluster<Point>::type;
		MPI_Gather(
			clusters.data(), k, clusterType,
			remoteClusters.data(), k, clusterType,
			0, communicator
		);

		if (id == 0)
		{
			// Fuse clusters and commit changes
			for (uint64 i = 0; i < remoteClusters.size(); i++)
				clusters[i % k].fuse(clusters[i]);

			for (uint64 i = 0; i < k; i++)
				clusters[i].commit();
		}
	}

	void finalize() {

		if (id == 0)
			memberships = std::vector<uint32>(dataset.size());

		// Gather local memberships [Optional: root could aswell calculate all the memberships]
		MPI_Gatherv(
			localMemberships.data(), receiveCount, MPI_UNSIGNED,
			memberships.data(), sendCounts.data(), displacements.data(), MPI_UNSIGNED,
			0, communicator
		);
	}

	void writeResults() const {
		if (id == 0) {

			std::string outFilename;
			if (CommandLine::get().getValue("output", outFilename))
			{
				CsvWriter<float32> writer(outFilename);
				writer.write(dataset, memberships);
			}
		}
	}

	bool checkConvergence(float32 oldLoss, float32 newLoss) const {
		return fabs(oldLoss - newLoss) <= tolerance;
	}

	bool hasConverged() const {
		return converged;
	}
};