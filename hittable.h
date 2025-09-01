#pragma once

// class containing hit information
#include "ray.h"
#include "vec3.h"
class hit_record {
public:
    point3 p; // hit point
    vec3 normal; // normal vector
    double t; // time of hit
    bool front_face;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// class for hittable objects
class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const = 0;
};
