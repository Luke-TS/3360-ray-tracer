#pragma once

#include "main.h"
#include "interval.h"
#include "ray.h"

/*
* Axis-Aligned Bounding Boxes
* Box consisting of 3 intervals along the x, y, and z axis.
* Used to create Bounding Volume Hierarchies.
*/
class aabb {
public:
    interval x, y ,z;

    // intervals are empty by default
    aabb() {};

    // initialize intervals
    aabb(const interval& x, const interval& y, const interval& z): x(x), y(y), z(z) {}

    // create box from 2 points
    aabb(const point3& a, const point3& b) {
        x = (a[0] <= b[0]) ? interval(a[0], b[0]) : interval(b[0], a[0]);
        y = (a[1] <= b[1]) ? interval(a[1], b[1]) : interval(b[1], a[1]);
        z = (a[2] <= b[2]) ? interval(a[2], b[2]) : interval(b[2], a[2]);
    }

    // create box overlapping 2 boxes
    aabb(const aabb& a, const aabb& b) {
        x = interval(a.x, b.x);
        y = interval(a.y, b.y);
        z = interval(a.z, b.z);
    }

    // 0:x 1:y 2:z
    const interval& axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    vec3 min() const {
        return vec3(x.min, y.min, z.min);
    }

    vec3 max() const {
        return vec3(x.max, y.max, z.max);
    }

    bool hit(const ray& r, interval ray_t) const {
        //g_num_box_tests++;
        const point3& ray_orig = r.origin();
        const vec3&   ray_dir  = r.direction();

        for (int axis = 0; axis < 3; axis++) {
            const interval& ax = axis_interval(axis);
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
