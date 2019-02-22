#pragma once

#include "core_types.h"
#include "mpi_globals.h"
#include "templates/singleton.h"
#include "containers/point.h"
#include "containers/cluster.h"
#include "utils/command_line.h"
#include "utils/csv_parser.h"
#include "utils/csv_writer.h"
#include "utils/scoped_lock.h"
#include "utils/data_generator.h"
#include "omp/critical_section.h"

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
	std::vector<int32> memberships;

	/// Clusters critical sections
	std::vector<OMP::CriticalSection> clusterGuards;

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
	FORCE_INLINE void run()
	{
		// Default values
		uint32 numClusters = 5;
		std::string initMethod;

		// Read from command line
		auto & gCommandLine = CommandLine::get();
		gCommandLine.getValue("num-clusters", numClusters);
		gCommandLine.getValue("init-method", initMethod);

		// Init cluster guards
		clusterGuards.resize(numClusters);

		// Compute initial clusters setup
		if (rank == 0)
		{
			/* Read command line */
			if (initMethod == "furthest")
				clusters = cluster::initFurthest(globalDataset, numClusters);
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
			// Create parser
			CsvParser<T> parser(filename);
			globalDataset = parser.parse();

			// Init memberships
			memberships.resize(globalDataset.size());
		}

		loadDataset();
	}

	/// Create test dataset and send points to other nodes
	FORCE_INLINE void createDataset()
	{
		// Dataset creation options
		uint32
			numDataPoints	= 1024,
			dataDim			= 2,
			numClusters		= 5;
		
		// Read from command line
		auto & commandLine = CommandLine::get();
		commandLine.getValue("gen-num", numDataPoints);
		commandLine.getValue("gen-dim", dataDim);
		commandLine.getValue("num-clusters", numClusters);

		{
			// Create data generator
			DataGenerator<T> generator(numDataPoints, numClusters, dataDim);
			globalDataset = generator.generate();

			// Init memberships
			memberships.resize(globalDataset.size());
		}

		loadDataset();
	}

	/// @todo Write dataset to disk
	FORCE_INLINE void writeDataset(const std::string & filename)
	{
		if (rank == 0)
		{
			CsvWriter<T> writer(filename);
			writer.write(localDataset, memberships);
		}
	}

	/// Load dataset on other nodes
	void loadDataset()
	{
		const uint32 numDataPoints	= globalDataset.size();
		const uint32 commSize		= MPI::getCommSize(communicator);

		// Get data chunks
		int32 receiveCount = 0;
		std::vector<int32> dataChunks = getDataChunks(numDataPoints, commSize);
		
		// Tell each node how many points it will receive
		MPI_Scatter(
			dataChunks.data(), commSize, MPI::DataType<int32>::type,
			&receiveCount, commSize, MPI::DataType<int32>::type,
			0, communicator
		);

		// Init vectors size
		localDataset.resize(receiveCount);
		memberships.resize(receiveCount);

		// Compute scatterv displacements
		std::vector<int32> displacements(commSize, 0);
		for (int32 d = 0, i = 0; d < numDataPoints; d += dataChunks[i++])
			displacements[i] = d;

		// Scatter points among nodes
		const auto pointDataType = point::type;
		MPI_Scatterv(
			globalDataset.data(), dataChunks.data(), displacements.data(), pointDataType,
			localDataset.data(), receiveCount, pointDataType,
			0, communicator
		);
	}

protected:
	/// Optimization routine
	void optimize()
	{
		const uint32 numClusters = clusters.size();
		const uint32 numDataPoints = localDataset.size();

		#pragma omp parallel
		{
			// Private copy of clusters
			auto threadClusters = clusters;

			#pragma for nowait
			for (uint64 i = 0; i < numDataPoints; ++i)
			{
				const auto & p = localDataset[i];

				// Take dist of first cluster
				uint32 clusterIdx = 0;
				float32 minDist = clusters[0].getDistance(p);

				// Find closest cluster
				for (uint32 k = 1; k < numClusters; ++k)
				{
					float32 dist = threadClusters[k].getDistance(p);
					if (dist < minDist) minDist = dist, clusterIdx = k;
				}
				
				threadClusters[clusterIdx].addWeight(p, 1.f);

				// Update local membership
				memberships[i] = clusterIdx;
			}

			#pragma omp for
			for (uint32 k = 0; k < numClusters; ++k)
			{
				ScopedLock<OMP::CriticalSection> _(&clusterGuards[k]);
				clusters[k].fuse(threadClusters[k]);
			}
		}
	}

	/// Update local clusters
	FORCE_INLINE void updateLocalClusters()
	{
		// @todo Broadcast updated clusters
		MPI_Bcast(clusters.data(), clusters.size(), cluster::type, 0, communicator);
	}

	/// Gather remote clusters and fuse with global clusters
	void updateGlobalClusters()
	{
		const uint32 commSize		= MPI::getCommSize(communicator);
		const uint32 numClusters	= clusters.size();

		// Init remote clusters vector
		std::vector<cluster> remoteClusters(commSize * numClusters);

		// @todo Gather remote clusters
		const auto clusterDataType = cluster::type;
		MPI_Gather(
			clusters.data(), numClusters, clusterDataType,
			remoteClusters.data(), numClusters, clusterDataType,
			0, communicator
		);

		if (rank == 0)
		{
			// Fuse clusters
			for (uint32 i = numClusters; i < remoteClusters.size(); ++i)
				clusters[i % numClusters].fuse(remoteClusters[i]);
			
			// Commit changes
			for (uint32 i = 0; i < clusters.size(); ++i)
				clusters[i].commit();
		}
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