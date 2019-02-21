#pragma once

#include "core_types.h"
#include "mpi_globals.h"
#include "templates/singleton.h"
#include "containers/point.h"
#include "containers/cluster.h"

#include <string>
#include <vector>

/**
 * @class Node public/node.h
 * 
 * A node that executes a distributed kmeans algorithm
 */
template<typename T>
class Node : Singleton<Node<T>>
{
public:
	/// Point type
	using point = Point<T, POINT_MAX_SIZE>;

	/// Cluster type
	using cluster = Cluster<point>;

protected:
	/// Node communicator
	MPI_Comm communicator;

	/// Node rank
	int32 rank;

	/// Global dataset
	std::vector<point> globalDataset;

	/// Node local points
	std::vector<point> localDataset;

	/// Node local clusters
	std::vector<cluster> clusters;

	/// Node local vector of memberships
	std::vector<uint32> memberships;

public:
	/// Default constructor
	FORCE_INLINE Node(MPI_Comm _communicator = MPI_COMM_WORLD) :
		communicator(_communicator),
		rank(-1)
	{
		// Get rank
		MPI_Comm_rank(communicator, &rank);

		// Create required data types
		point::createMpiType();
		cluster::createMpiType();
	}

	//////////////////////////////////////////////////
	// Node API
	//////////////////////////////////////////////////

	/// Run algorithm
	FORCE_INLINE void run(/* Node setup */)
	{
		// Default values
		uint32 numClusters = 5;
		std::string initMethod;

		// Compute initial clusters setup
		if (rank == 0)
		{
			/* Read command line */
			if (initMethod == "furthest")
				;
			else
				clusters = cluster::initRandom(globalDataset, numClusters);
		}

		// Optimization loop
		for (uint32 epoch = 0; epoch < 100; ++epoch)
		{
			// Get updated clusters
			updateLocalClusters();

			// Optimize clusters
			optimize();

			// Gather remote centroids and update global
			updateGlobalClusters();
		}
	}
	
	/// Import dataset and send points to other nodes
	FORCE_INLINE void readDataset(const std::string & filename)
	{
		if (rank == 0)
		{
			/* @todo Read input file */
		}

		loadDataset();
	}

	/// @todo Write dataset to disk
	FORCE_INLINE void writeDataset(const std::string & filename)
	{

	}

	/// Load dataset on other nodes
	void loadDataset()
	{
		const uint32 numDataPoints	= localDataset.size();
		const uint32 commSize		= MPI::getCommSize(communicator);

		// Get data chunks
		int32 receiveCount = 0;
		std::vector<int32> dataChunks = getDataChunks(globalDataset.size(), commSize);
		
		// Tell each node how many points it will receive
		MPI_Scatter(
			dataChunks.data(), 1, MPI::DataType<int32>::type,
			&receiveCount, 1, MPI::DataType<int32>::type,
			0, communicator
		);

		// Init vectors size
		localDataset.resize(receiveCount);
		memberships.resize(receiveCount);

		// Compute scatterv displacements
		std::vector<int32> displacements(commSize, 0);
		for (int32 d = 0, i = 0; d < numDataPoints; d += dataChunks[i++])
			displacements[i] = d;

		// @todo Scatter points among nodes
		const auto pointDataType = point::type;
		MPI_Scatterv(
			globalDataset.data(), dataChunks.data(), displacements.data(), pointDataType,
			localDataset.data(), receiveCount, pointDataType,
			0, communicator
		);
	}

protected:
	/// @todo Optimization routine
	void optimize()
	{
		/*  */
	}

	/// Gather remote clusters and fuse with global clusters
	void updateGlobalClusters()
	{
		const uint32 commSize		= MPI::getCommSize(communicator);
		const uint32 numClusters	= clusters.size();

		// Init remote clusters vector
		std::vector<cluster> remoteClusters(commSize * numClusters);

		// @todo Gather remote clusters
		const auto clusterDataType = 0;
		MPI_Gather(
			clusters.data(), numClusters, clusterDataType,
			remoteClusters.data(), numClusters, clusterDataType,
			0, communicator
		);

		if (rank == 0)
		{
			// Fuse clusters
			for (uint32 i = 0; i < remoteClusters.size(); ++i)
				clusters[i % numClusters].fuse(remoteClusters[i]);
			
			// Commit changes
			for (uint32 i = 0; i < remoteClusters.size(); ++i)
				clusters[i].commit();
		}
	}

	/// Update local clusters
	FORCE_INLINE void updateLocalClusters()
	{
		// @todo Broadcast updated clusters
		MPI_Bcast(clusters.data(), clusters.size(), MPI_AINT, 0, communicator);
	}

	/// Get data splits
	static FORCE_INLINE std::vector<int32> getDataChunks(uint64 numDataPoints, uint32 numNodes)
	{
		// Assign points
		const uint64 perNode = numDataPoints / numNodes;
		std::vector<int32> chunks(numNodes, perNode);

		// Assign remaining points
		uint64 remaining = numDataPoints - perNode;
		for (uint32 i = 0; remaining > 0; ++i, --remaining)
			++chunks[i];
		
		return chunks;
	}
};