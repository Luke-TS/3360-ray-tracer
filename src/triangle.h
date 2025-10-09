#pragma once

#include "hittable.h"
#include "interval.h"
#include "main.h"
#include "vec3.h"
#include "ray.h"
#include <memory>

class triangle : public Hittable {
public:
    int gpu_index;

    triangle(const point3& a, const point3& b, const point3& c, std::shared_ptr<material> mat) 
        : a(a), b(b), c(c), mat(mat) {
        // per-axis min and max
        point3 min_point(
            std::fmin(a.x(), std::fmin(b.x(), c.x())),
            std::fmin(a.y(), std::fmin(b.y(), c.y())),
            std::fmin(a.z(), std::fmin(b.z(), c.z()))
        );

        point3 max_point(
            std::fmax(a.x(), std::fmax(b.x(), c.x())),
            std::fmax(a.y(), std::fmax(b.y(), c.y())),
            std::fmax(a.z(), std::fmax(b.z(), c.z()))
        );

        // pad slightly in case of axis-aligned triangles
        const double eps = 1e-6f;
        min_point += -vec3(eps, eps, eps);
        max_point += vec3(eps, eps, eps);

        bbox = aabb(min_point, max_point);
    }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        //g_num_primitive_tests++;
        const float kEpsilon = 1e-6f;  // geometric tolerance

        vec3 edge1 = b - a;
        vec3 edge2 = c - a;

        // Möller–Trumbore
        vec3 pvec = cross(r.direction(), edge2);
        float det = dot(edge1, pvec);

        // parallel ray?
        if (fabs(det) < kEpsilon)
            return false;

        float inv_det = 1.0f / det;
        vec3 tvec = r.origin() - a;

        // barycentric u
        float u = dot(tvec, pvec) * inv_det;
        if (u < 0.0f || u > 1.0f)
            return false;

        // barycentric v
        vec3 qvec = cross(tvec, edge1);
        float v = dot(r.direction(), qvec) * inv_det;
        if (v < 0.0f || (u + v) > 1.0f)
            return false;

        // t 
        float t = dot(edge2, qvec) * inv_det;

        // check range
        if (t < ray_t.min || t > ray_t.max)
            return false;

        // valid hit 
        rec.t = t;
        rec.p = r.at(t);
        rec.mat = mat;

        // normal
        vec3 outward_norm = cross(edge1, edge2);
        rec.set_face_normal(r, unit_vector(outward_norm));

        return true;
    }

    virtual aabb bounding_box() const override {
        return bbox;
    }

    virtual int type_id() const override {
        return HITTABLE_TRIANGLE;
    }

    virtual int object_index() const override {
        return gpu_index;
    }

    virtual void set_object_index(int i) override {
        gpu_index = i;
    }

private:
    point3 a, b, c;
    std::shared_ptr<material> mat;
    aabb bbox;
};

