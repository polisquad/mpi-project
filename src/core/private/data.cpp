#include <data.h>
#include <numeric>
#include <algorithm>
#include "utils.h"

using std::vector;

Data::Data(const vector<float> &attributes) : attributes(attributes)
{
    cls = "";
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
    for (int i = 0; i < row.size() - 1; ++i)
    {
        x = std::atof(row[i].c_str());
        attributes.push_back(x);
    }
    cls = row[row.size() - 1];
}

/**
 * Calculates the N-dimensional euclidian distance between this Data object and another one, computing the value w.r.t
 * the attributes
 * @param other
 * @return
 */
float Data::getDistance(const Data &other) const
{
    float squaredSum = 0;
    for (int i = 0; i < size(); ++i)
    {
        squaredSum += pow(attributes[i] - other[i], 2);
    };
    return sqrt(squaredSum);
}

std::vector<Data> Data::readCSVFileNormalized(std::string const &filename)
{
    vector<Data> dataset;
    vector<float> x;
    std::ifstream file(filename);
    Data lowest;
    Data highest;

    CSVRow row;
    while (file >> row)
    {
        if (row.size() > 0)
        {
            const Data &data = Data(row);
            dataset.emplace_back(data);
            lowest.setLowest(data);
            highest.setHighest(data);
        }
    }
    normalizeData(dataset, lowest, highest);
    return dataset;
}

float const &Data::operator[](size_t index) const
{
    return attributes[index];
}

unsigned long Data::size() const
{
    return attributes.size();
}

Data Data::operator+(Data const &other)
{
    vector<float> attributes;
    std::transform(begin(), end(), other.begin(), std::back_inserter(attributes), std::plus<>());
    return Data(attributes);
}

Data Data::operator/(float const &d)
{
    vector<float> attributes;
    std::transform(begin(), end(), std::back_inserter(attributes), [d](float x)
    { return x / d; });
    return Data(attributes);
}

vector<float, std::allocator<float>>::const_iterator Data::begin() const
{
    return attributes.begin();
}

vector<float, std::allocator<float>>::const_iterator Data::end() const
{
    return attributes.end();
}

Data Data::operator+=(Data const &other)
{
    if (attributes.empty())
    {
        attributes = other.attributes;
    } else
    {
        vector<float> result;
        std::transform(begin(), end(), other.begin(), std::back_inserter(result), std::plus<>());
        attributes = result;
    }
    return *this;
}

bool Data::operator==(Data const &other)
{
    return std::equal(begin(), end(), other.begin());
}

const string &Data::getCls() const
{
    return cls;
}

/**
 * Updates the attributes picking the minimum values from this and the other Data
 * @param other
 * @return
 */
void Data::setLowest(Data const &other)
{
    vector<float> result;
    if (attributes.empty())
    {
        attributes = other.attributes;
    } else
    {
        std::transform(begin(), end(), other.begin(), std::back_inserter(result),
                       [](float x, float y)
                       { return std::min(x, y); });
        attributes = result;
    }
}

void Data::setHighest(Data const &other)
{
    vector<float> result;
    if (attributes.empty())
    {
        attributes = other.attributes;
    } else
    {
        std::transform(begin(), end(), other.begin(), std::back_inserter(result),
                       [](float x, float y)
                       { return std::max(x, y); });
        attributes = result;
    }
}

void Data::normalize(Data const &min_values, Data const &max_values)
{
    for (int i = 0; i < attributes.size(); ++i)
    {
        attributes[i] = (attributes[i] - min_values[i]) / (max_values[i] - min_values[i]);
    }
}

void Data::normalizeData(vector<Data> &dataset, Data const &min_values, Data const &max_values)
{
    std::for_each(dataset.begin(), dataset.end(), [min_values, max_values](Data &v)
    {
        v.normalize(min_values, max_values);
    });
}

Data::Data()
= default;
