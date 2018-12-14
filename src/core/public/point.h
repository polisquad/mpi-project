#pragma once

#include "core_types.h"
#include "async/mpi.h"

/**
 * @struct Point point.h
 * @brief A 2D point mainly used for experimenting
 */
template<typename T = float32>
struct Point
{
public:
    union
    {
        /// @brief Coordinates buffer
        T data[2];

        /// @brief Single components
        /// @{
        struct
        {
            T x, y;
        };
        /// @}
    };

public:
    /// @brief Default-constructor
    FORCE_INLINE Point() = default;

    /// @brief Copy-constructor
    FORCE_INLINE Point(const Point<T> &p) = default;

    /// @brief Components-constructor
    FORCE_INLINE Point(T _x, T _y) : x(_x), y(_y)
    {}

    /// @brief Buffer-constructor
    FORCE_INLINE Point(T _data[2])
    { memcpy(data, _data, sizeof(data)); }

    /// @brief Assignment-operator
    FORCE_INLINE Point &operator=(const Point<T> &p) = default;

    /// @brief Return squared length of point (from origin)
    FORCE_INLINE T getSquaredSize() const
    { return (x * x) + (y * y); }

    /// @brief Compare points
    /// @{
    FORCE_INLINE bool operator==(const Point<T> &p)
    { return fabsf(x - p.x) <= (FLT_EPSILON * 2.f) & fabsf(y - p.y) <= (FLT_EPSILON * 2.f); }

    FORCE_INLINE bool operator!=(const Point<T> &p)
    { return fabsf(x - p.x) > (FLT_EPSILON * 2.f) | fabsf(y - p.y) > (FLT_EPSILON * 2.f); }
    /// @}

    /**
     * @brief Point/Point operators
     *
     * @param [in] p other point
     *
     * @return Point result of operation
     * @{
     */
    FORCE_INLINE Point operator+(const Point<T> &p) const
    { return Point(x + p.x, y + p.y); }

    FORCE_INLINE Point operator-(const Point<T> &p) const
    { return Point(x - p.x, y - p.y); }

    FORCE_INLINE Point operator*(const Point<T> &p) const
    { return Point(x * p.x, y * p.y); }

    FORCE_INLINE Point operator/(const Point<T> &p) const
    { return Point(x / p.x, y / p.y); }

    FORCE_INLINE Point &operator+=(const Point<T> &p)
    {
        x += p.x, y += p.y;
        return *this;
    }

    FORCE_INLINE Point &operator-=(const Point<T> &p)
    {
        x -= p.x, y -= p.y;
        return *this;
    }

    FORCE_INLINE Point &operator*=(const Point<T> &p)
    {
        x *= p.x, y *= p.y;
        return *this;
    }

    FORCE_INLINE Point &operator/=(const Point<T> &p)
    {
        x /= p.x, y /= p.y;
        return *this;
    }
    /// @}

    /**
     * @brief Point/scalar operators
     *
     * @param [in] p other point
     *
     * @return Point result of operation
     * @{
     */
    FORCE_INLINE Point operator+(T s) const
    { return Point(x + s, y + s); }

    FORCE_INLINE Point operator-(T s) const
    { return Point(x - s, y - s); }

    FORCE_INLINE Point operator*(T s) const
    { return Point(x * s, y * s); }

    FORCE_INLINE Point operator/(T s) const
    { return Point(x / s, y / s); }

    friend FORCE_INLINE Point operator+(T s, const Point<T> &p)
    { return Point(p.x + s, p.y + s); }

    friend FORCE_INLINE Point operator-(T s, const Point<T> &p)
    { return Point(p.x - s, p.y - s); }

    friend FORCE_INLINE Point operator*(T s, const Point<T> &p)
    { return Point(p.x * s, p.y * s); }

    friend FORCE_INLINE Point operator/(T s, const Point<T> &p)
    { return Point(p.x / s, p.y / s); }
    /// @}

    /// @brief Returns smallest point
    friend FORCE_INLINE Point<T> min(const Point<T> &a, const Point<T> &b)
    { return a.x + a.y < b.x + b.y ? a : b; }

    /// @brief Return squared distance between two points
    /// @{
    FORCE_INLINE T getSquaredDistance(const Point<T> &p) const
    { return operator-(p).getSquaredSize(); }

    FORCE_INLINE T getDistance(const Point<T> &p) const
    { return sqrtf(getSquaredDistance(p)); }
    /// @}

    /**
     * @brief Print point to stream
     *
     * @param [in] stream output stream
     */
    inline void print(FILE *stream = stdout) const;
};

/////////////////////////////////////////////////
// Float32 specialization                      //
/////////////////////////////////////////////////

typedef Point<float32> point;
