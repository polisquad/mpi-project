#pragma once

#include "core_types.h"
#include "point.h"

/**
 * @class Point containers/point.h
 * 
 * @ref Point vector intrinsics specialization
 */
template<typename T, uint32 N>
struct GCC_ALIGN(32) Point<T, N, true> : MPI::DataType<Point<T, N>>
{
public:
	/// Vector operations class
	using VecOps = SIMD::Vector<T, N>;

	/// Vector intrinsics data type
	using VecT = typename VecOps::Type;

protected:
	union
	{
		/// Vector intrinsics data
		VecT data;

		/// Array data representation
		T array[8];
	};

	/// Actual size of the data
	uint32 size;

public:
	/// Default constructor
	explicit FORCE_INLINE Point(uint32 _size = N) :
		data{},
		size(_size > 0 ? _size : N) {}
	
protected:
	/// Data cosntructor
	explicit FORCE_INLINE Point(VecT _data, uint32 _size = N) :
		data(_data),
		size(_size > 0 ? _size : N) {}

public:
	/// Random access operator
	/// @{
	FORCE_INLINE T &		operator[](uint32 i)		{ return array[i]; }
	FORCE_INLINE const T &	operator[](uint32 i) const	{ return array[i]; }
	/// @}

	/// Returns dimension of point
	FORCE_INLINE uint32 getNum() const
	{
		return size;
	}

	/// Returns squared size
	FORCE_INLINE float32 getSquaredSize() const
	{
		return VecOps::convert(VecOps::template dp<0xf1>(data, data));
	}

	/// Returns squared size
	FORCE_INLINE float32 getSize() const
	{
		return VecOps::convert(VecOps::sqrt(VecOps::template dp<0xf1>(data, data)));
	}

	/// Returns squared distance between two points
	FORCE_INLINE float32 getDistance(const Point & p) const
	{
		const VecT temp = VecOps::sub(data, p.data);
		return VecOps::convert(VecOps::sqrt(VecOps::template dp<0xf1>(temp, temp)));
	}

	/**
	 * Point-point compound assignments
	 * 
	 * @param [in] p point operand
	 * @return self
	 * @{
	 */
	FORCE_INLINE Point & operator+=(const Point & p)
	{
		// Add vectors
		size = size < p.size ? size : p.size;
		data = VecOps::add(data, p.data);

		return *this;
	}

	FORCE_INLINE Point & operator-=(const Point & p)
	{
		// Sub vectors
		size = size < p.size ? size : p.size;
		data = VecOps::sub(data, p.data);

		return *this;
	}

	FORCE_INLINE Point & operator*=(const Point & p)
	{
		// Sub vectors
		size = size < p.size ? size : p.size;
		data = VecOps::mul(data, p.data);

		return *this;
	}

	FORCE_INLINE Point & operator/=(const Point & p)
	{
		// Sub vectors
		size = size < p.size ? size : p.size;
		data = VecOps::div(data, p.data);

		return *this;
	}
	/// @}

	/**
	 * Point-scalar compound assignments
	 * 
	 * @param [in] s scalar operand
	 * @return self
	 * @{
	 */
	FORCE_INLINE Point & operator+=(typename ConstRef<T>::Type s)
	{
		data = VecOps::add(data, s);
		return *this;
	}

	FORCE_INLINE Point & operator-=(typename ConstRef<T>::Type s)
	{
		data = VecOps::sub(data, s);
		return *this;
	}

	FORCE_INLINE Point & operator*=(typename ConstRef<T>::Type s)
	{
		data = VecOps::mul(data, s);
		return *this;
	}

	FORCE_INLINE Point & operator/=(typename ConstRef<T>::Type s)
	{
		data = VecOps::div(data, s);
		return *this;
	}
	/// @}

	/**
	 * Point-point arithmetic operations
	 * 
	 * @param [in] p point operand
	 * @return self
	 * @{
	 */
	FORCE_INLINE Point operator+(const Point & p)
	{
		return Point(VecOps::add(data, p.data), size < p.size ? size : p.size);
	}

	FORCE_INLINE Point operator-(const Point & p)
	{
		return Point(VecOps::sub(data, p.data), size < p.size ? size : p.size);
	}

	FORCE_INLINE Point operator*(const Point & p)
	{
		return Point(VecOps::mul(data, p.data), size < p.size ? size : p.size);
	}

	FORCE_INLINE Point operator/(const Point & p)
	{
		return Point(VecOps::div(data, p.data), size < p.size ? size : p.size);
	}
	/// @}

	/**
	 * Point-point arithmetic operations
	 * 
	 * @param [in] p point operand
	 * @return self
	 * @{
	 */
	FORCE_INLINE Point operator+(typename ConstRef<T>::Type s)
	{
		return Point(VecOps::add(data, s), size);
	}

	FORCE_INLINE Point operator-(typename ConstRef<T>::Type s)
	{
		return Point(VecOps::sub(data, s), size);
	}

	FORCE_INLINE Point operator*(typename ConstRef<T>::Type s)
	{
		return Point(VecOps::mul(data, s), size);
	}

	FORCE_INLINE Point operator/(typename ConstRef<T>::Type s)
	{
		return Point(VecOps::div(data, s), size);
	}
	/// @}

	/**
	 * Print point
	 * 
	 * @param out out stream
	 */
	FORCE_INLINE void print(FILE * out = stdout) const;

	//////////////////////////////////////////////////
	// MPI Interface
	//////////////////////////////////////////////////
	
	/// Creates the MPI datatype, if not already created
	static FORCE_INLINE MPI_Datatype createMpiType()
	{
		const int32 blockSize[] = {N, 1};
		const MPI_Aint blockDisplacement[] = {0, offsetof(Point, size)};
		const MPI_Datatype blockType[] = {MPI::DataType<T>::type, MPI::DataType<uint32>::type};

		MPI_Type_create_struct(2, blockSize, blockDisplacement, blockType, &Point::type);

		// Align size to 32 Bytes
		MPI_Type_create_resized(Point::type, 0, sizeof(Point), &Point::type);

		// Commit type
		MPI_Type_commit(&Point::type);
		return Point::type;
	}
};