#include <iosfwd>
#include <csv.h>
#include <vector>

#include "csv.h"

using std::string;
using std::istream;
using std::stringstream;
using std::vector;

string const &CSVRow::operator[](size_t index) const
{
    return m_data[index];
}

size_t CSVRow::size() const
{
    return m_data.size();
}

void CSVRow::readNextRow(istream &str)
{
    string line;
    getline(str, line);

    stringstream lineStream(line);
    string cell;

    m_data.clear();
    while (getline(lineStream, cell, ','))
    {
        m_data.push_back(cell);
    }
    // This checks for a trailing comma with no data after it.
    if (!lineStream && cell.empty())
    {
        // If there was a trailing comma then add an empty element.
        m_data.push_back("");
    }
}

istream &operator>>(istream &str, CSVRow &data)
{
    data.readNextRow(str);
    return str;
}