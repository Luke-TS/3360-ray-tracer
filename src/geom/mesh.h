#pragma once

#include "hittable.h"
#include "scene/scene.h"
#include "triangle.h"

#include <memory>
#include <vector>
#include <array>

namespace rt::geom {

class Mesh : public Hittable {
public:
    scene::Scene tris;

    Mesh(const std::vector<core::Point3>& vertices,
                  const std::vector<std::array<int,3>>& indices,
                  std::shared_ptr<material::Material> mat) {
        for( const auto& face : indices ) {
            auto tri = std::make_shared<Triangle>(
                vertices[face[0]],
                vertices[face[1]],
                vertices[face[2]],
                mat
            );
            tris.Add(tri);
        }
    }

    virtual bool Hit(const core::Ray& r, core::Interval ray_t, HitRecord& rec) const override {
        return tris.Hit(r, ray_t, rec);
    }

    virtual Aabb BoundingBox() const override {
        return tris.BoundingBox();
    }
};

} // namespace rt::geom
