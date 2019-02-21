#pragma once

#include "core_types.h"
#include "containers/point.h"

#include <vector>

/**
 * @class CsvParser utils/csv_parser.h
 * 
 * A parser for csv files
 * All columns are assumed to be of
 * a fixed type T
 * 
 * Results is returned as a vector
 * of points of the appropriate size
 */
template<typename T, uint32 N = POINT_MAX_SIZE>
class CsvParser
{
public:
	/// Point data type
	using point = Point<T, N>;

protected:
	/// Holds the file
	FILE * fp;

	/// Number of rows
	uint32 rows;

	/// Number of values per row
	uint32 cols;

public:
	/// Default constructor
	/// @{
	explicit CsvParser(const char * filename) : fp(fopen(filename, "r"))
	{
		if (fp)
		{
			// Get number of lines
			uint64 lines = 1;

			char cc;
			while ((cc = fgetc(fp)) != EOF)
				if (cc == '\n') ++lines;

			rewind(fp);

			// Get number of cols
			uint64 values = 1;

			char line[256];
			if (fgets(line, 256, fp))
			{
				for (uint32 i = 0; line[i] != '\n' && i < 256; ++i)
					if (line[i] == ',') ++values;
			}

			// Set rows and cols
			rows = lines,
			cols = values;
		}
	}

	explicit FORCE_INLINE CsvParser(const std::string & filename) : CsvParser(filename.c_str()) {}
	/// @}

	/// Destructor
	FORCE_INLINE ~CsvParser()
	{
		if (fp) fclose(fp);
	}

	/// Parse file
	FORCE_INLINE std::vector<point> parse(uint32 start = 0, uint32 end = (uint32)-1)
	{
		std::vector<point> out; out.reserve(rows);

		char line[256];
		for(uint64 i = 0; fgets(line, 256, fp) && i < end; ++i)
			if (i >= start)
				out.push_back(parseLine(line));
		
		return out;
	}

protected:
	/// Parse line
	point parseLine(const char * line)
	{
		point out(cols);
		const char * buffer = line;

		uint32 i = 0; do
		{
			// Get value and increment buffer
			T val = T();
			buffer += parseValue(buffer, val);

			// Add to out vector
			out[i++] = val;
		} while (*(buffer++) == ',' && i < cols);

		return out;
	}

	/// Parse value
	uint32 parseValue(const char * row, T & val);
};

/// Parse specialization for common types
/// @{
template<>
FORCE_INLINE uint32 CsvParser<float32>::parseValue(const char * row, float32 & val)
{
	int32 cread = 0;
	sscanf(row, "%f%n", &val, &cread);

	return cread;
}

template<>
FORCE_INLINE uint32 CsvParser<float64>::parseValue(const char * row, float64 & val)
{
	int32 cread = 0;
	sscanf(row, "%lf%n", &val, &cread);

	return cread;
}
/// @}