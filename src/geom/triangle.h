#pragma once

#include "core/interval.h"
#include "core/vec3.h"
#include "core/ray.h"
#include "core/math_utils.h"

#include "hittable.h"

#include <memory>

namespace rt::geom {

class Triangle : public Hittable {
public:
    int gpu_index;

    Triangle(const core::Point3& a, const core::Point3& b, const core::Point3& c, std::shared_ptr<material::Material> mat) 
        : a_(a), b_(b), c_(c), mat_(mat) {
        // per-axis min and max
        core::Point3 min_point(
            std::fmin(a.x(), std::fmin(b.x(), c.x())),
            std::fmin(a.y(), std::fmin(b.y(), c.y())),
            std::fmin(a.z(), std::fmin(b.z(), c.z()))
        );

        core::Point3 max_point(
            std::fmax(a.x(), std::fmax(b.x(), c.x())),
            std::fmax(a.y(), std::fmax(b.y(), c.y())),
            std::fmax(a.z(), std::fmax(b.z(), c.z()))
        );

        // pad slightly in case of axis-aligned triangles
        const double eps = 1e-6f;
        min_point += -core::Vec3(eps, eps, eps);
        max_point += core::Vec3(eps, eps, eps);

        bbox_ = Aabb(min_point, max_point);
    }

    virtual bool Hit(const core::Ray& r, core::Interval ray_t, HitRecord& rec) const override {
        //g_num_primitive_tests++;
        const float kEpsilon = 1e-6f;  // geometric tolerance

        core::Vec3 edge1 = b_ - a_;
        core::Vec3 edge2 = c_ - a_;

        // Möller–Trumbore
        core::Vec3 pvec = core::Cross(r.direction(), edge2);
        float det = core::Dot(edge1, pvec);

        // parallel ray?
        if (fabs(det) < kEpsilon)
            return false;

        float inv_det = 1.0f / det;
        core::Vec3 tvec = r.origin() - a_;

        // barycentric u
        float u = core::Dot(tvec, pvec) * inv_det;
        if (u < 0.0f || u > 1.0f)
            return false;

        // barycentric v
        core::Vec3 qvec = core::Cross(tvec, edge1);
        float v = core::Dot(r.direction(), qvec) * inv_det;
        if (v < 0.0f || (u + v) > 1.0f)
            return false;

        // t 
        float t = Dot(edge2, qvec) * inv_det;

        // check range
        if (t < ray_t.min_ || t > ray_t.max_)
            return false;

        // valid hit 
        rec.t = t;
        rec.p = r.at(t);
        rec.mat = mat_;

        // normal
        core::Vec3 outward_norm = core::Cross(edge1, edge2);
        rec.set_face_normal(r, core::Normalize(outward_norm));

        return true;
    }

    virtual Aabb BoundingBox() const override {
        return bbox_;
    }

    virtual int TypeId() const override {
        return HITTABLE_TRIANGLE;
    }

    virtual int ObjectIndex() const override {
        return gpu_index;
    }

    virtual void set_object_index(int i) override {
        gpu_index = i;
    }

private:
    core::Point3 a_, b_, c_;
    std::shared_ptr<material::Material> mat_;
    Aabb bbox_;
};

} // namespace rt::geom
