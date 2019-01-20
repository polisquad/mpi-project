#ifndef MPI_PROJECT_POINT_H
#define MPI_PROJECT_POINT_H

#include <float.h>
#include "core_types.h"
#include <iostream>
#include <math.h>


/**
* @struct Point point.h
* @brief A 2D point.
*/
template<typename T = float32>
struct Point {
    T x, y;

    /// @brief Default-constructor
    Point() = default;

    /// @brief Copy-constructor
    Point(const Point<T>& p) = default;

    /// @brief Components-constructor
    Point(T _x, T _y) : x(_x), y(_y) {}

    /// @brief Assignment-operator
    Point &operator=(const Point<T>& p) = default;

    /// @brief Return squared length of point (from origin)
    T getSquaredSize() const { return (x * x) + (y * y); }

    /// @brief Compare points
    /// @{
    bool operator==(const Point<T>& p) {
        return fabsf(x - p.x) <= (FLT_EPSILON * 2.f) & fabsf(y - p.y) <= (FLT_EPSILON * 2.f);
    }

    bool operator!=(const Point<T>& p) {
        return fabsf(x - p.x) > (FLT_EPSILON * 2.f) | fabsf(y - p.y) > (FLT_EPSILON * 2.f);
    }
    /// @}

    /**
    * @brief Point/Point operators
    *
    * @param [in] p other point
    *
    * @return Point result of operation
    * @{
    */
    Point operator+(const Point<T>& p) const { return Point(x + p.x, y + p.y); }

    Point operator-(const Point<T>& p) const { return Point(x - p.x, y - p.y); }

    Point operator*(const Point<T>& p) const { return Point(x * p.x, y * p.y); }

    Point operator/(const Point<T>& p) const { return Point(x / p.x, y / p.y); }

    Point& operator+=(const Point<T>& p) {
        x += p.x, y += p.y;
        return *this;
    }

    Point& operator-=(const Point<T>& p) {
        x -= p.x, y -= p.y;
        return *this;
    }

    Point& operator*=(const Point<T>& p) {
        x *= p.x, y *= p.y;
        return *this;
    }

    Point& operator/=(const Point<T>& p) {
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
    Point operator+(T s) const { return Point(x + s, y + s); }

    Point operator-(T s) const { return Point(x - s, y - s); }

    Point operator*(T s) const { return Point(x * s, y * s); }

    Point operator/(T s) const { return Point(x / s, y / s); }

    Point operator/=(T s) {x /= s; y /= s;}

    friend Point operator+(T s, const Point<T>& p) { return Point(p.x + s, p.y + s); }

    friend Point operator-(T s, const Point<T>& p) { return Point(p.x - s, p.y - s); }

    friend Point operator*(T s, const Point<T>& p) { return Point(p.x * s, p.y * s); }

    friend Point operator/(T s, const Point<T>& p) { return Point(p.x / s, p.y / s); }
    /// @}

    /// @brief Returns smallest point
    friend Point<T> min(const Point<T>& a, const Point<T>& b) { return a.x + a.y < b.x + b.y ? a : b; }

    /// @brief Return squared distance between two points
    /// @{
    T getSquaredDistance(const Point<T>& p) const { return operator-(p).getSquaredSize(); }

    T getDistance(const Point<T>& p) const { return sqrtf(getSquaredDistance(p)); }
    /// @}

    /**
    * @brief Print point to stream
    *
    * @param [in] stream output stream
    */
    void print(FILE* stream = stdout) const;
};

/////////////////////////////////////////////////
// Float32 specialization                      //
/////////////////////////////////////////////////

typedef Point<float32> point;

template<>
void Point<float32>::print(FILE* stream) const {
    fprintf(stream, "p2(%.6f, %.6f)\n", x, y);
}

#endif //MPI_PROJECT_POINT_H
