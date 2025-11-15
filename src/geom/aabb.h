#pragma once

#include "core/interval.h"
#include "core/vec3.h"
#include "core/ray.h"

namespace rt::geom {

class Aabb {
public:
    core::Interval x, y ,z;

    // intervals are empty by default
    Aabb() {}

    // initialize intervals
    Aabb(const core::Interval& x, const core::Interval& y, const core::Interval& z)
        : x(x), y(y), z(z) {}

    // create box from 2 points
    Aabb(const core::Point3& a, const core::Point3& b) {
        x = (a[0] <= b[0]) ? core::Interval(a[0], b[0]) : core::Interval(b[0], a[0]);
        y = (a[1] <= b[1]) ? core::Interval(a[1], b[1]) : core::Interval(b[1], a[1]);
        z = (a[2] <= b[2]) ? core::Interval(a[2], b[2]) : core::Interval(b[2], a[2]);
    }

    // create box overlapping 2 boxes
    Aabb(const Aabb& a, const Aabb& b) {
        x = core::Interval(a.x, b.x);
        y = core::Interval(a.y, b.y);
        z = core::Interval(a.z, b.z);
    }

    // Expand an existing box to include a point
    Aabb(const Aabb& box, const core::Vec3& p) {
        x = core::Interval(
            std::min(box.x.min_, p.x()),
            std::max(box.x.max_, p.x())
        );
        y = core::Interval(
            std::min(box.y.min_, p.y()),
            std::max(box.y.max_, p.y())
        );
        z = core::Interval(
            std::min(box.z.min_, p.z()),
            std::max(box.z.max_, p.z())
        );
    }

    // 0:x 1:y 2:z
    const core::Interval& axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    core::Vec3 min() const {
        return { x.min_, y.min_, z.min_ };
    }

    core::Vec3 max() const {
        return { x.max_, y.max_, z.max_ };
    }

    core::Vec3 center() const {
        return core::Vec3(
            0.5 * (x.min_ + x.max_),
            0.5 * (y.min_ + y.max_),
            0.5 * (z.min_ + z.max_)
        );
    }

    // RETURN 0, 1, or 2
    int LongestAxis() const {
        double dx = x.max_ - x.min_;
        double dy = y.max_ - y.min_;
        double dz = z.max_ - z.min_;

        if (dx >= dy && dx >= dz) return 0;
        if (dy >= dz) return 1;
        return 2;
    }

    // Needed for SAH
    double SurfaceArea() const {
        double dx = x.max_ - x.min_;
        double dy = y.max_ - y.min_;
        double dz = z.max_ - z.min_;
        return 2.0 * (dx*dy + dy*dz + dz*dx);
    }

    bool Hit(const core::Ray& r, core::Interval ray_t) const {
        const core::Point3& ray_orig = r.origin();
        const core::Vec3&   ray_dir  = r.direction();

        for (int axis = 0; axis < 3; axis++) {
            const core::Interval& ax = axis_interval(axis);
            const double adinv = 1.0 / ray_dir[axis];

            double t0 = (ax.min_ - ray_orig[axis]) * adinv;
            double t1 = (ax.max_ - ray_orig[axis]) * adinv;

            if (t0 < t1) {
                if (t0 > ray_t.min_) ray_t.min_ = t0;
                if (t1 < ray_t.max_) ray_t.max_ = t1;
            } else {
                if (t1 > ray_t.min_) ray_t.min_ = t1;
                if (t0 < ray_t.max_) ray_t.max_ = t0;
            }

            if (ray_t.max_ <= ray_t.min_)
                return false;
        }
        return true;
    }
};

} // namespace rt::geom
