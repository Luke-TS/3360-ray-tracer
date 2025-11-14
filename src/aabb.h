#pragma once

#include "interval.h"
#include "ray.h"

/*
* Axis-Aligned Bounding Boxes
* Box consisting of 3 intervals along the x, y, and z axis.
* Used to create Bounding Volume Hierarchies.
*/
class Aabb {
public:
    Interval x, y ,z;

    // intervals are empty by default
    Aabb() {};

    // initialize intervals
    Aabb(const Interval& x, const Interval& y, const Interval& z): x(x), y(y), z(z) {}

    // create box from 2 points
    Aabb(const Point3& a, const Point3& b) {
        x = (a[0] <= b[0]) ? Interval(a[0], b[0]) : Interval(b[0], a[0]);
        y = (a[1] <= b[1]) ? Interval(a[1], b[1]) : Interval(b[1], a[1]);
        z = (a[2] <= b[2]) ? Interval(a[2], b[2]) : Interval(b[2], a[2]);
    }

    // create box overlapping 2 boxes
    Aabb(const Aabb& a, const Aabb& b) {
        x = Interval(a.x, b.x);
        y = Interval(a.y, b.y);
        z = Interval(a.z, b.z);
    }

    // 0:x 1:y 2:z
    const Interval& axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    Vec3 min() const {
        return Vec3(x.min, y.min, z.min);
    }

    Vec3 max() const {
        return Vec3(x.max, y.max, z.max);
    }

    bool hit(const Ray& r, Interval ray_t) const {
        //g_num_box_tests++;
        const Point3& ray_orig = r.origin();
        const Vec3&   ray_dir  = r.direction();

        for (int axis = 0; axis < 3; axis++) {
            const Interval& ax = axis_interval(axis);
            const double adinv = 1.0 / ray_dir[axis];

            // determine interval intersection points
            auto t0 = (ax.min - ray_orig[axis]) * adinv;
            auto t1 = (ax.max - ray_orig[axis]) * adinv;

            if (t0 < t1) {
                if (t0 > ray_t.min) ray_t.min = t0;
                if (t1 < ray_t.max) ray_t.max = t1;
            } else {
                if (t1 > ray_t.min) ray_t.min = t1;
                if (t0 < ray_t.max) ray_t.max = t0;
            }

            if (ray_t.max <= ray_t.min)
                return false;
        }
        return true;
    }
};
