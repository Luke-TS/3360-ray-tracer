#pragma once

#include "vec3.h"

// simple ray class consisting of a point and a direction vector
// ray moves one direction vector for every unit time
class Ray {
public:
    Ray() {}

    Ray(const Point3& origin, const Vec3& direction) : orig(origin), dir(direction) {}

    const Point3& origin() const { return orig ;}
    const Vec3& direction() const { return dir;}

    /**
    * ray::at(t) returns the position of the ray at time t (seconds)
    */
    Point3 at(double t) const {
        return orig + t*dir;
    }

private:
    Point3 orig;
    Vec3 dir;
};
