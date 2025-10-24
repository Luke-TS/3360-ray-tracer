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

    // surface coordinates of hit point for texture mapping
    double u; 
    double v;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

enum HittableType {
    HITTABLE_SPHERE = 0,
    HITTABLE_TRIANGLE = 1,
    HITTABLE_SQUARE = 2,
};;

// class for hittable objects
class Hittable {
public:
    virtual ~Hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual aabb bounding_box() const = 0;

    virtual int type_id() const = 0;
    virtual int object_index() const = 0;
    virtual void set_object_index(int i) = 0;
};
