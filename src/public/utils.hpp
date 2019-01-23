#ifndef MPI_PROJECT_UTILS_HPP
#define MPI_PROJECT_UTILS_HPP

#include <point.hpp>
#include <core_types.h>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>

// temporary utils for trying stuff

void writeDatasetToFile(const std::vector<Point<float32>> &dataset, const char *fileName = "../data/in.csv") {
    FILE *fp = fopen(fileName, "w");
    if (!fp) throw std::invalid_argument("input file not valid");

    for (auto &p: dataset)
        fprintf(fp, "%f,%f,0\n", p.x, p.y);

    fclose(fp);
}

void writeResultsToFile(const std::vector<uint64>& memberships,
                        const char *in = "../data/in.csv",
                        const char *out = "../data/out.csv") {
    FILE *fp = fopen(in, "r");
    if (!fp) throw std::invalid_argument("input file not valid");

    FILE *fp_out = fopen(out, "w");
    if (!fp_out) throw std::invalid_argument("output file not valid");

    float32 x, y;
    uint64 processed = 0;
    while (fscanf(fp, "%f,%f,%*f", &x, &y) > 0) {
        fprintf(fp_out, "%f,%f,%llu\n", x, y, memberships[processed]);
        processed++;
    }

    fclose(fp);
    fclose(fp_out);
}

std::vector<Point<float32>> generateDummyDataset(const uint64 datasetSize = 1024 * 4) {
    std::default_random_engine generator;
    generator.seed(std::random_device()());
    std::uniform_real_distribution<float32> distribution(0.0, 1.0);

    auto dataset = std::vector<Point<float32>>(datasetSize);
    for (uint64 i = 0; i < datasetSize; ++i) {
        point p(6, 6);
        do {
            p = point(distribution(generator) * 10,
                      distribution(generator) * 10);
        } while (sinf(p.y) * sinf(p.y) + cosf(p.x) * cosf(p.y) < 0.2f | p.x * p.y < 9.f);
        dataset[i] = p;
    }

    return dataset;
}

/**
 * @return random uint64 between 0 and max(excluded)
 */
uint64 randomNextInt(uint64 max) {
    std::default_random_engine generator;
    generator.seed(std::random_device()());
    std::uniform_int_distribution<uint64> distribution(0, max - 1);
    return distribution(generator);
}

std::vector<Point<float32>> loadDataset(const char *fileName = "../data/in.csv") {
    FILE *fp = fopen(fileName, "r");
    if (!fp) throw std::invalid_argument("input file not valid");

    std::vector<Point<float32>> data;
    float32 x, y;
    while (fscanf(fp, "%f,%f,%*f", &x, &y) > 0) {
        data.emplace_back(x, y);
    }

    fclose(fp);
    return data;
}


std::vector<int32> getDataSplits(uint64 dataSize, int32 numNodes) {
    std::vector<int32> dataSplits(numNodes, 0);
    int32 perProcess = static_cast<int32 >(std::floor(dataSize / numNodes));
    uint64 remaining = dataSize - perProcess * numNodes;

    for (int &dataSplit : dataSplits)
        dataSplit += perProcess;

    if (remaining > 0) {
        // Assign remaining ones starting from workers
        uint64 curr = dataSplits.size() - 1;
        for (int32 i = 0; i < remaining; i += 1) {
            dataSplits[curr] += 1;
            curr -= 1;
        }
    }
    return dataSplits;
}
#endif //MPI_PROJECT_UTILS_HPP
