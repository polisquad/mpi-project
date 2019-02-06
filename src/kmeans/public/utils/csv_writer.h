#pragma once

#include "coremin.h"
#include "alg/vector.h"

/**
 * Very simple csv writer
 */
template<typename T>
class CsvWriter
{
protected:
	/// File pointer
	FILE * fp;

public:
	/// Read constructor
	FORCE_INLINE CsvWriter(const std::string & filename) :
		fp(fopen(filename.c_str(), "w")) {}

	/// Destructor
	FORCE_INLINE ~CsvWriter()
	{
		// Release file resource
		fclose(fp);
	}

	/// Write values
	int32 write(const std::vector<Vec<T>> & data)
	{
		if (!fp) return -1;

		int32 rows = 0;
		for (const auto & point : data)
		{
			writeRow(point);
			++rows;
		}

		return rows;
	}

	/// Write clusterization results
	/// @{
	int32 write(const std::vector<std::vector<Vec<T>>> & groups)
	{
		if (!fp) return -1;

		int32 rows = 0;
		for (uint32 k = 0; k < groups.size(); ++k)
		{
			for (const auto & point : groups[k])
			{
				writeRow(point, k);
				++rows;
			}
		}

		return rows;
	}
	int32 write(const std::vector<Vec<T>> & dataset, const std::vector<uint32> & memberships)
	{
		if (!fp) return -1;

		int32 i = 0;
		for (; i < dataset.size(); ++i)
			writeRow(dataset[i], memberships[i]);

		return i;
	}
	/// @}

protected:
	/// Write single row
	void writeRow(const Vec<T> & point);
	
	/// Write single row with integer tag
	void writeRow(const Vec<T> & point, int32 tag);
	
	/// Write single row with string tag
	void writeRow(const Vec<T> & point, const std::string & tag);
};

template<>
FORCE_INLINE void CsvWriter<float32>::writeRow(const Vec<float32> & point)
{
	uint32 i = 0;
	for (; i < point.getSize() - 1; ++i)
		fprintf(fp, "%.3f,", point[i]); fprintf(fp, "%.3f\n", point[i]);
}

template<>
FORCE_INLINE void CsvWriter<float32>::writeRow(const Vec<float32> & point, int32 tag)
{
	uint32 i = 0;
	for (; i < point.getSize(); ++i)
		fprintf(fp, "%.3f,", point[i]); fprintf(fp, "%d\n", tag);
}