#pragma once

#include "ray.h"

// class containing hit information
class hit_record {
public:
    point3 p; // hit point
    vec3 normal; // normal vector
    double t; // time of hit
};

// class for hittable objects
class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const = 0;
};
