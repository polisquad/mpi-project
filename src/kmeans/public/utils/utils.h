#pragma once

#include <vector>

std::vector<int32> getDataSplits(uint64 dataSize, int32 numNodes)
{
    std::vector<int32> dataSplits(numNodes, 0);

    int32 perProcess = dataSize / numNodes;
    uint64 remaining = dataSize - perProcess * numNodes;

    for (int & dataSplit : dataSplits)
        dataSplit += perProcess;

    if (remaining > 0)
	{
        // Assign remaining ones starting from workers
        uint64 curr = dataSplits.size() - 1;
        for (int32 i = 0; i < remaining; i += 1, curr -= 1)
            dataSplits[curr] += 1;
    }
    return dataSplits;
}

void writeResultsToFile(const std::vector<uint32> & memberships,
                        const char *in = "../data/in.csv",
                        const char *out = "../data/out.csv") {
    FILE *fp = fopen(in, "r");
    if (!fp) throw std::invalid_argument("input file not valid");

    FILE *fp_out = fopen(out, "w");
    if (!fp_out) throw std::invalid_argument("output file not valid");

    float32 x, y;
    uint64 processed = 0;
    while (fscanf(fp, "%f,%f,%*f", &x, &y) > 0) {
        fprintf(fp_out, "%f,%f,%u\n", x, y, memberships[processed]);
        processed++;
    }

    fclose(fp);
    fclose(fp_out);
}