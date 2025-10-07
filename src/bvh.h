#pragma once

#include "aabb.h"
#include "hittable.h"
#include "scene.h"
#include "triangle_mesh.h"

#include <algorithm>

class bvh_node : public Hittable {
  public:
    bvh_node(triangle_mesh& mesh) : bvh_node(mesh.tris) {}

    bvh_node(Scene& list) : bvh_node(list.objects, 0, list.objects.size()) {
        // There's a C++ subtlety here. This constructor (without span indices) creates an
        // implicit copy of the hittable list, which we will modify. The lifetime of the copied
        // list only extends until this constructor exits. That's OK, because we only need to
        // persist the resulting bounding volume hierarchy.
    }

    bvh_node(std::vector<shared_ptr<Hittable>>& objects, size_t start, size_t end) {
        auto axis = random_int(0, 2);
        auto comparator = (axis == 0) ? box_x_compare
            : (axis == 1) ? box_y_compare
            : box_z_compare;

        size_t object_span = end - start;

        if (object_span <= 4) {
            // leaf node - just store the primitives
            for (size_t i = start; i < end; i++) {
                primitives.push_back(objects[i]);
            }
            left = nullptr;
            right = nullptr;
        } else {
            // sort by comparator and split
            std::sort(objects.begin() + start, objects.begin() + end, comparator);

            auto mid = start + object_span / 2;
            left  = std::make_shared<bvh_node>(objects, start, mid);
            right = std::make_shared<bvh_node>(objects, mid, end);
        }

        // compute bounding box
        if (!primitives.empty()) {
            // leaf: merge all prim AABBs
            aabb temp_box;
            bool first_box = true;
            for (auto& obj : primitives) {
                aabb b = obj->bounding_box();
                temp_box = first_box ? b : aabb(temp_box, b);
                first_box = false;
            }
            bbox = temp_box;
        } else {
            bbox = aabb(left->bounding_box(), right->bounding_box());
        }
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!bbox.hit(r, ray_t)) {
            return false;
        }

        bool hit_anything = false;
        hit_record temp_rec;
        auto closest_so_far = ray_t.max;

        // check for leaf node
        if (!primitives.empty()) {
            for (auto& obj : primitives) {
                if (obj->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
            }
            return hit_anything;
        }

        // internal node
        bool hit_left = left->hit(r, ray_t, rec);
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    std::vector<std::shared_ptr<Hittable>> primitives;
    shared_ptr<Hittable> left;
    shared_ptr<Hittable> right;
    aabb bbox;

    static bool box_compare(
        const shared_ptr<Hittable> a, const shared_ptr<Hittable> b, int axis_index
    ) {
        auto a_axis_interval = a->bounding_box().axis_interval(axis_index);
        auto b_axis_interval = b->bounding_box().axis_interval(axis_index);
        return a_axis_interval.min < b_axis_interval.min;
    }

    static bool box_x_compare (const shared_ptr<Hittable> a, const shared_ptr<Hittable> b) {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare (const shared_ptr<Hittable> a, const shared_ptr<Hittable> b) {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare (const shared_ptr<Hittable> a, const shared_ptr<Hittable> b) {
        return box_compare(a, b, 2);
    }
};
