#ifndef MPI_PROJECT_LOCAL_CENTROID_HPP
#define MPI_PROJECT_LOCAL_CENTROID_HPP

#include <core_types.h>
#include <point.hpp>

struct LocalCentroid {
    Point<float32> point;
    bool isZeroed;

    LocalCentroid() = default;
    LocalCentroid(Point<float32> p, bool isZeroed) : point(p), isZeroed(isZeroed) {}
};
#endif //MPI_PROJECT_LOCAL_CENTROID_HPP
