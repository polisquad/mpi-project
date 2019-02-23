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