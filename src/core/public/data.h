#include <vector>
#include <string>

#include "csv.h"

#ifndef MPI_PROJECT_DATA_H
#define MPI_PROJECT_DATA_H

class Data
{
public:
    explicit Data(const CSVRow &row);

    float const &operator[](size_t index) const;

    unsigned long size();

    virtual float getEuclidianDistance(const Data &other);

    static std::vector<Data> readCSVFile(std::string const &filename);

private:
    std::vector<float> attributes;
    std::string cls;
};

#endif //MPI_PROJECT_DATA_H
