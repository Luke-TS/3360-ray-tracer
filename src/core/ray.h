#pragma once

#include "vec3.h"

namespace rt::core {

class Ray {
public:
    Ray() {}

    Ray(const Point3& origin, const Vec3& direction) : orig_(origin), dir_(direction) {}

    const Point3& origin() const { return orig_ ;}
    const Vec3& direction() const { return dir_;}

    /**
    * ray::at(t) returns the position of the ray at time t (seconds)
    */
    Point3 at(double t) const {
        return orig_ + t*dir_;
    }

private:
    Point3 orig_;
    Vec3 dir_;
};

} // namespace rt::core
