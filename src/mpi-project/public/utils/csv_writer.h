#pragma once

#include "core_types.h"
#include "containers/point.h"

#include <vector>

/**
 * @class CsvWriter utils/csv_writer.h
 * 
 * A writer for csv files
 * All columns are assumed to be of
 * a fixed type T
 */
template<typename T, uint32 N = POINT_MAX_SIZE>
class CsvWriter
{
public:
	/// Point data type
	using point = Point<T, N>;

protected:
	/// Holds the file
	FILE * fp;
	
public:
	/// Default constructor
	/// @{
	explicit CsvWriter(const char * filename) : fp(fopen(filename, "w")) {} 
	explicit FORCE_INLINE CsvWriter(const std::string & filename) : CsvWriter(filename.c_str()) {}
	/// @}

	/// Destructor
	FORCE_INLINE ~CsvWriter()
	{
		if (fp) fclose(fp);
	}

	/// Write csv file
	uint32 FORCE_INLINE write(const std::vector<point> & values, const std::vector<int32> & tags)
	{
		int32 rows = 0;
		for (; rows < values.size(); ++rows)
			writeLine(values[rows], tags[rows]);
		
		return rows;
	}

protected:
	/// Write single row
	void writeLine(const point & value, int32 tag);
};

/// @ref writeLine() specialization
/// @{
template<>
FORCE_INLINE void CsvWriter<float32>::writeLine(const point & value, int32 tag)
{
	for(uint32 i = 0;  i < value.getNum(); ++i)
		fprintf(fp, "%.3f,", value[i]);
	fprintf(fp, "%d\n", tag);
}

template<>
FORCE_INLINE void CsvWriter<float64>::writeLine(const point & value, int32 tag)
{
	for(uint32 i = 0;  i < value.getNum(); ++i)
		fprintf(fp, "%.3lf,", value[i]);
	fprintf(fp, "%d\n", tag);
}
/// @}