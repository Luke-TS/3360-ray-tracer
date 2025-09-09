#pragma once

#include <memory>

// class containing hit information
//#include "material.h"
#include "aabb.h"
#include "ray.h"
#include "vec3.h"
#include "interval.h"

// to solve circular references betwixt material and hittable code
class material;

class hit_record {
public:
    point3 p; // hit point
    vec3 normal; // normal vector
    std::shared_ptr<material> mat;
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

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual aabb bounding_box() const = 0;
};
