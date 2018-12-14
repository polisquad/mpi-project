#include <vector>
#include <string>

#include "csv.h"

#ifndef MPI_PROJECT_DATA_H
#define MPI_PROJECT_DATA_H

class Data
{
public:
    Data();

    explicit Data(const CSVRow &row);

    explicit Data(const vector<float> &attributes);

    float const &operator[](size_t index) const;

    unsigned long size() const;

    vector<float, std::allocator<float>>::const_iterator begin() const;

    vector<float, std::allocator<float>>::const_iterator end() const;

    virtual float getDistance(const Data &other) const;

    static std::vector<Data> readCSVFile(std::string const &filename);

    Data operator+(Data const &other);

    Data operator/(float const &d);

    Data operator+=(Data const &other);

    bool operator==(Data const &other);

    const string &getCls() const;

private:
    std::vector<float> attributes;
    std::string cls;
};

#endif //MPI_PROJECT_DATA_H
