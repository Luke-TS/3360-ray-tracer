#pragma once

#include "aabb.h"
#include "ray.h"
#include "vec3.h"
#include "interval.h"

// to solve circular references between material and hittable code
class Material;

class HitRecord {
public:
    bool hit;
    Point3 p; // hit point
    Vec3 normal; // normal vector
    shared_ptr<Material> mat;
    double t; // time of hit
    bool front_face;

    // surface coordinates of hit point for texture mapping
    double u; 
    double v;

    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }

    HitRecord() = default;
    HitRecord(const HitRecord&) = default;
    HitRecord(HitRecord&&) = default;
    HitRecord& operator=(const HitRecord&) = default;   // <-- important
    HitRecord& operator=(HitRecord&&) = default;        // <-- important
    ~HitRecord() = default;
};

enum HittableType {
    HITTABLE_SPHERE = 0,
    HITTABLE_TRIANGLE = 1,
    HITTABLE_SQUARE = 2,
};;

class Hittable {
public:
    virtual ~Hittable() = default;

    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const = 0;

    virtual Aabb bounding_box() const = 0;

    virtual int type_id() const = 0;
    virtual int object_index() const = 0;
    virtual void set_object_index(int i) = 0;
};
