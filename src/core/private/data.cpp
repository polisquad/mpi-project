#include <data.h>
#include <csv.h>
#include <numeric>

#include "data.h"
#include "utils.h"

using std::vector;

/**
 * Calculates the N-dimensional euclidian distance between this Data object and another one, computing the value w.r.t
 * the attributes
 * @param other
 * @return
 */
float Data::getEuclidianDistance(const Data &other)
{
    float squaredSum = 0;
    for (int i = 0; i < size(); ++i)
    {
        squaredSum += pow(attributes[i] - other[i], 2);
    };
    return sqrt(squaredSum);
}

std::vector<Data> Data::readCSVFile(std::string const &filename)
{
    vector<Data> out;
    vector<float> x;
    std::ifstream file(filename);

    CSVRow row;
    while (file >> row)
    {
        out.emplace_back(Data(row));
    }
    return out;
}

float const &Data::operator[](size_t index) const
{
    return attributes[index];
}

unsigned long Data::size()
{
    return attributes.size();
}

/**
 * Builds a Data element from a CSV row. The row MUST have the following format:
 * a1, .., an, k --> where a1..an are float values and k is the class of this row.
 * See the famous IRIS dataset for example.
 * @param row The CSV row to parse
 */
Data::Data(const CSVRow &row)
{
    float x;
    for (int i = 0; i < row.size()-1; ++i)
    {
        x = std::atof(row[i].c_str());
        attributes.push_back(x);
    }
    cls = row[row.size()-1];
}
