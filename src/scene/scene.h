#pragma once

#include <memory>
#include <vector>

#include "core/ray.h"
#include "core/interval.h"

#include "geom/aabb.h"
#include "geom/aabb.h"
#include "geom/hittable.h"

namespace rt::scene {

class Scene : public geom::Hittable {
 public:
  std::vector<std::shared_ptr<rt::geom::Hittable>> objects_;

  Scene() = default;

  explicit Scene(std::shared_ptr<geom::Hittable> object) {
    Add(object);
  }

  // Add an object to the scene
  void Add(const std::shared_ptr<geom::Hittable>& object) {
    if (!object) return;

    objects_.push_back(object);

    // Update bounding box
    bbox_ = geom::Aabb(bbox_, object->BoundingBox());
  }

  // Remove all objects
  void Clear() {
    objects_.clear();
    bbox_ = geom::Aabb();  // reset to empty box (assuming default is empty)
  }

  // Access to scene objects
  const std::vector<std::shared_ptr<geom::Hittable>>& Objects() const {
    return objects_;
  }

  // Hittable interface
  bool Hit(const core::Ray& r, core::Interval ray_t, geom::HitRecord& rec) const override {
    geom::HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max_;

    for (const auto& object : objects_) {
      if (object->Hit(r, core::Interval(ray_t.min_, closest_so_far), temp_rec)) {
        hit_anything = true;
        closest_so_far = temp_rec.t;
        rec = temp_rec;
      }
    }

    return hit_anything;
  }

    geom::Aabb BoundingBox() const override {
    return bbox_;
  }

  // Unused for Scene
  int TypeId() const override { return -1; }
  int ObjectIndex() const override { return -1; }
  void set_object_index(int) override {}

 private:
    geom::Aabb bbox_;
};

}  // namespace rt::scene
