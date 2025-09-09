#pragma once

#include "vec3.h"

// simple ray class consisting of a point and a direction vector
// ray moves one direction vector for every unit time
class ray {
public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

    const point3& origin() const { return orig ;}
    const vec3& direction() const { return dir;}

    /**
    * ray::at(t) returns the position of the ray at time t (seconds)
    */
    point3 at(double t) const {
        return orig + t*dir;
    }

private:
    point3 orig;
    vec3 dir;
};
