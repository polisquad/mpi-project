# OpenMP/MPI

Implement the **k-means clustering algorithm** in OpenMP/MPI, trying to maximize the performance (reduce the execution time) by carefully exploiting the resources within one computing node with multiple processing cores (OpenMP) and across computing nodes (MPI).

> **Optional:** implement the same algorithm in Apache Flink and compare the performance of the two implementations (processing time and scalability) under various workloads.

## Usage

```bash
$ bin/mpi-project --help

Usage: mpi-project [input] [output] [options]

Options:
  --output {filename}       overrides output file
  --num-clusters {num}      sets number of clusters (default: 5)
  --init-method {method}    cluster initialization method ('random' or 'furthest', default: 'random')
  --num-epochs {num}        maximum number of epochs to simulate (default: 100)
  --gen-num {num}           if no input is specified, number of data points to generate (default: 1024)
  --gen-dim {num}           if no input is specified, dimension of the generated data (default: 2)
```

## Implementation

The outline of the k-means algorithm is reported here:

```
init:
	init MPI
	read command line arguments
	*read/generate dataset
	scatter dataset

run:
	compute initial clusters
	loop:
		broadcast current global clusters
		optimize:
			for each local point:
				find closest cluster
				add weight to closest cluster
				set point membership
		gather remote clusters
		*compute current global cluster
	gather memberships
	
end:
	[*write output file]
	shutdown MPI
```

> steps with `*` are processed only by the root node

### Where to extract parallelism?

We used MPI, OpenMP and vectorization to extract parallelism at different levels:

- **MPI**: each node is assigned a subset of points. A node (`rank == 0`) is elected as root node.
At the end of each epoch the root node gathers updated clusters from other nodes and computes the current global cluster by merging them together. Then broadcast back the updated global cluster.

- **OpenMP**: we use OpenMP to compute the updated local clusters. Each thread is assigned a subset of points and mantains each private copy of the local clusters. After a thread as finished processing is points, it updates the shared local cluster copy. To prevent race condition a mutex (one for each cluster) is locked before updating the shared cluster.

- **vectorization**: we used vectorization at point operations level. All point operations are vectorized using intel AVX instructions.

### Design choices

...