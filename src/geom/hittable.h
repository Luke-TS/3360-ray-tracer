#pragma once

#include "core/ray.h"
#include "core/vec3.h"
#include "core/interval.h"

#include "aabb.h"

#include <memory>

// to solve circular references between material and hittable code
namespace rt::material {
    class Material;
}

namespace rt::geom {

class HitRecord {
public:
    bool hit;
    core::Point3 p; // hit point
    core::Vec3 normal; // normal vector
    std::shared_ptr<material::Material> mat;
    double t; // time of hit
    bool front_face;

    // surface coordinates of hit point for texture mapping
    double u; 
    double v;

    void set_face_normal(const core::Ray& r, const core::Vec3& outward_normal) {
        front_face = core::Dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }

    HitRecord() = default;
    HitRecord(const HitRecord&) = default;
    HitRecord(HitRecord&&) = default;
    HitRecord& operator=(const HitRecord&) = default;   // <-- important
    HitRecord& operator=(HitRecord&&) = default;        // <-- important
    ~HitRecord() = default;
};

// used to identify object type for GPU intersection testing
enum HittableType {
    HITTABLE_SPHERE = 0,
    HITTABLE_TRIANGLE = 1,
    HITTABLE_SQUARE = 2,
};;

class Hittable {
public:
    virtual ~Hittable() = default;

    virtual bool Hit(const core::Ray& r, core::Interval ray_t, HitRecord& rec) const = 0;

    virtual Aabb BoundingBox() const = 0;

    virtual int TypeId() const = 0;
    virtual int ObjectIndex() const = 0;
    virtual void set_object_index(int i) = 0;
};

} // namespace rt::geom
