#ifndef MPI_PROJECT_CSV_H
#define MPI_PROJECT_CSV_H

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using std::string;
using std::stringstream;
using std::istream;
using std::vector;

class CSVRow
{
public:
    string const &operator[](size_t index) const;

    size_t size() const;

    void readNextRow(istream &str);

private:
    std::vector<std::string> m_data;
};

std::istream &operator>>(std::istream &str, CSVRow &data);

#endif //MPI_PROJECT_CSV_H
