#pragma once

#include "coremin.h"
#include "alg/vector.h"

/**
 * A simple csv parser
 */
template<typename T>
class CsvParser
{
protected:
	/// File pointer
	FILE * fp;

	/// Number of rows
	uint64 rows;

	/// Number of values per row
	uint64 cols;

public:
	/// Read constructor
	FORCE_INLINE CsvParser(const std::string & filename) :
		fp(fopen(filename.c_str(), "r"))
	{
		if (fp != nullptr)
		{
			// Get number of rows
			uint64 lines = 0;
			char cc;
			while ((cc = fgetc(fp)) != EOF)
				if (cc == '\n') ++lines;

			rewind(fp);

			// Get number of cols
			uint64 values = 1;
			char line[256];
			if (fgets(line, 256, fp))
			{
				for (uint32 i = 0; i < 256; ++i)
					if (line[i] == ',') ++values;
			}

			rewind(fp);

			rows = lines;
			cols = values;
		}
	}

	/// Destructor
	FORCE_INLINE ~CsvParser()
	{
		// Release file resource
		fclose(fp);
	}

	/// Returns number of rows
	FORCE_INLINE uint64 getRowCount() const { return rows; }

	/// Returns number of values
	FORCE_INLINE uint64 getColCount() const { return cols; }

	/// Parse values
	FORCE_INLINE Array<Vec<T>> parse(uint64 start = 1, uint64 end = -1)
	{
		Array<Vec<T>> out(rows);

		uint64 i = 0;
		char line[256];

		while (fgets(line, 256, fp))
		{
			if (i >= start & i < end)
				out.push(parseRow(line));

			++i;
		}

		return out;
	}

protected:
	/// Parse a single row
	FORCE_INLINE Vec<T> parseRow(const char * row)
	{
		Vec<T> out(cols, 0.f);
		char * buffer = (char*)row;

		uint32 i = 0; do
		{
			// Get value and increment buffer
			T val = 0.f;
			buffer += parseValue(buffer, val);

			// Add to out vector
			out[i++] = val;
		} while (*(buffer++) == ',');

		return out;
	}

	/// Parse a single value
	uint32 parseValue(const char * row, T & val);
};

template<>
FORCE_INLINE uint32 CsvParser<float32>::parseValue(const char * row, float32 & val)
{
	int32 cread = 0;
	sscanf(row, "%f%n", &val, &cread);

	return cread;
}