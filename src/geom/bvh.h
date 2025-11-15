#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <limits>

#include "core/ray.h"
#include "core/interval.h"

#include "scene/scene.h"

#include "hittable.h"
#include "aabb.h"
#include "mesh.h"

namespace rt::geom {

/// Node layout usable on both CPU and GPU
struct BvhNodeGPU {
    Aabb     bbox;
    uint32_t left;    // internal: index of left child; leaf: first primitive index
    uint32_t right;   // internal: index of right child; leaf: primitive count
    uint32_t isLeaf;  // 1 = leaf, 0 = internal
};

/// SAH-built, flattened BVH that is a Hittable itself.
class Bvh : public Hittable {
  public:
    // Construct from a Scene
    Bvh(scene::Scene& scene)
        : Bvh(scene.objects_) {}

    // Construct from a mesh (triangle list)
    Bvh(Mesh& mesh)
        : Bvh(mesh.tris) {}

    // Construct from explicit list of objects
    Bvh(std::vector<std::shared_ptr<Hittable>>& objects)
    {
        if (objects.empty()) {
            root_index_ = -1;
            return;
        }

        // Copy primitives into our own storage
        primitives_ = objects;

        const int n = static_cast<int>(primitives_.size());
        prim_indices_.resize(n);
        prim_bounds_.resize(n);
        prim_centroids_.resize(n);

        for (int i = 0; i < n; ++i) {
            prim_indices_[i] = i;
            Aabb b = primitives_[i]->BoundingBox();
            prim_bounds_[i] = b;
            prim_centroids_[i] = b.center();
        }

        // Build SAH BVH (temporary pointer-based tree)
        std::unique_ptr<BuildNode> root_build =
            BuildSah(0, n);

        // Flatten into GPU/CPU-friendly nodes_
        nodes_.reserve(2 * n);
        root_index_ = flatten(*root_build);
    }

    /// Ray intersection (CPU traversal)
    bool Hit(const core::Ray& r, core::Interval ray_t, HitRecord& rec) const override {
        if (root_index_ < 0 || nodes_.empty())
            return false;

        bool hit_anything = false;
        double closest = ray_t.max_;
        HitRecord temp_rec;

        // Iterative traversal stack
        int stack[64];
        int sp = 0;
        stack[sp++] = root_index_;

        while (sp > 0) {
            int node_idx = stack[--sp];
            const BvhNodeGPU& node = nodes_[node_idx];

            core::Interval node_range(ray_t.min_, closest);
            if (!node.bbox.Hit(r, node_range))
                continue;

            if (node.isLeaf) {
                // Leaf: test primitives
                int first = static_cast<int>(node.left);
                int count = static_cast<int>(node.right);

                for (int i = 0; i < count; ++i) {
                    int prim_idx = prim_indices_[first + i];
                    auto& obj   = primitives_[prim_idx];
                    if (obj->Hit(r, core::Interval(ray_t.min_, closest), temp_rec)) {
                        hit_anything = true;
                        closest = temp_rec.t;
                        rec = temp_rec;
                    }
                }
            } else {
                // Internal: push children (push far first)
                int left  = static_cast<int>(node.left);
                int right = static_cast<int>(node.right);

                // Optional: order by which child is closer to the ray origin
                // For now, just push right then left.
                stack[sp++] = right;
                stack[sp++] = left;
            }
        }

        return hit_anything;
    }

    Aabb BoundingBox() const override {
        if (root_index_ < 0 || nodes_.empty())
            return Aabb(); // empty
        return nodes_[root_index_].bbox;
    }

    // ignore: primitive-only functions (match your old BvhNode)
    int TypeId() const override { return -1; }
    int ObjectIndex() const override { return -1; }
    void set_object_index(int) override {}

    // ==== GPU-facing accessors ====

    const std::vector<BvhNodeGPU>& nodes() const { return nodes_; }
    const std::vector<int>&        prim_indices() const { return prim_indices_; }
    const std::vector<std::shared_ptr<Hittable>>& primitives() const { return primitives_; }

  private:
    // === Internal build structures ===

    struct BuildNode {
        Aabb bounds;
        int  firstPrim = 0;  // offset in prim_indices_
        int  primCount = 0;  // >0 => leaf
        std::unique_ptr<BuildNode> left;
        std::unique_ptr<BuildNode> right;

        bool is_leaf() const { return primCount > 0; }
    };

    std::vector<std::shared_ptr<Hittable>> primitives_;   // actual geometry
    std::vector<int>    prim_indices_;    // index remapping
    std::vector<Aabb>   prim_bounds_;
    std::vector<core::Vec3>   prim_centroids_;

    std::vector<BvhNodeGPU> nodes_;       // flattened BVH
    int root_index_ = -1;

    static constexpr int   MAX_LEAF_SIZE = 4;
    static constexpr int   BIN_COUNT     = 16;
    static constexpr float TRAVERSAL_COST = 1.0f;
    static constexpr float INTERSECTION_COST = 1.0f;

    // === SAH build ===

    std::unique_ptr<BuildNode> BuildSah(int start, int end) {
        auto node = std::make_unique<BuildNode>();

        // Compute bounds of all primitives in [start, end)
        Aabb bounds;
        bool first = true;
        for (int i = start; i < end; ++i) {
            int idx = prim_indices_[i];
            if (first) {
                bounds = prim_bounds_[idx];
                first = false;
            } else {
                bounds = Aabb(bounds, prim_bounds_[idx]);
            }
        }
        node->bounds = bounds;
        int count = end - start;

        if (count <= MAX_LEAF_SIZE) {
            // Make leaf
            node->firstPrim = start;
            node->primCount = count;
            return node;
        }

        // Compute centroid bounds
        Aabb centroid_bounds;
        first = true;
        for (int i = start; i < end; ++i) {
            int idx = prim_indices_[i];
            const core::Vec3& c = prim_centroids_[idx];
            if (first) {
                centroid_bounds = Aabb(c, c);
                first = false;
            } else {
                centroid_bounds = Aabb(centroid_bounds, c);
            }
        }

        int axis = centroid_bounds.LongestAxis();
        double min_c = centroid_bounds.axis_interval(axis).min_;
        double max_c = centroid_bounds.axis_interval(axis).max_;
        double extent = max_c - min_c;

        if (extent <= 0.0) {
            // All centroids are on top of each other -> leaf
            node->firstPrim = start;
            node->primCount = count;
            return node;
        }

        // Binning for SAH
        struct Bin {
            int  count = 0;
            Aabb bounds;
            bool initialized = false;
        };

        Bin bins[BIN_COUNT];

        const double invExtent = 1.0 / extent;

        for (int i = start; i < end; ++i) {
            int idx = prim_indices_[i];
            double c = prim_centroids_[idx][axis];
            int b = static_cast<int>((c - min_c) * invExtent * BIN_COUNT);
            if (b < 0) b = 0;
            if (b >= BIN_COUNT) b = BIN_COUNT - 1;

            Bin& bin = bins[b];
            if (!bin.initialized) {
                bin.bounds = prim_bounds_[idx];
                bin.initialized = true;
            } else {
                bin.bounds = Aabb(bin.bounds, prim_bounds_[idx]);
            }
            bin.count++;
        }

        // Prefix and suffix SAH
        Aabb left_bounds[BIN_COUNT];
        int  left_count[BIN_COUNT];
        Aabb right_bounds[BIN_COUNT];
        int  right_count[BIN_COUNT];

        // Left-to-right prefix
        Aabb acc_bounds;
        int  acc_count = 0;
        bool acc_init  = false;
        for (int i = 0; i < BIN_COUNT; ++i) {
            if (bins[i].count > 0) {
                if (!acc_init) {
                    acc_bounds = bins[i].bounds;
                    acc_init = true;
                } else {
                    acc_bounds = Aabb(acc_bounds, bins[i].bounds);
                }
                acc_count += bins[i].count;
            }
            left_bounds[i] = acc_bounds;
            left_count[i]  = acc_count;
        }

        // Right-to-left suffix
        acc_init  = false;
        acc_count = 0;
        for (int i = BIN_COUNT - 1; i >= 0; --i) {
            if (bins[i].count > 0) {
                if (!acc_init) {
                    acc_bounds = bins[i].bounds;
                    acc_init = true;
                } else {
                    acc_bounds = Aabb(acc_bounds, bins[i].bounds);
                }
                acc_count += bins[i].count;
            }
            right_bounds[i] = acc_bounds;
            right_count[i]  = acc_count;
        }

        // Evaluate best split
        double best_cost = std::numeric_limits<double>::infinity();
        int best_split = -1;
        double parent_area = bounds.SurfaceArea();

        for (int i = 0; i < BIN_COUNT - 1; ++i) {
            if (left_count[i] == 0 || right_count[i + 1] == 0)
                continue;

            double left_area  = left_bounds[i].SurfaceArea();
            double right_area = right_bounds[i + 1].SurfaceArea();

            double cost = TRAVERSAL_COST +
                (left_area / parent_area)  * left_count[i]  * INTERSECTION_COST +
                (right_area / parent_area) * right_count[i + 1] * INTERSECTION_COST;

            if (cost < best_cost) {
                best_cost  = cost;
                best_split = i;
            }
        }

        // If SAH says "no benefit to split", make leaf
        double leaf_cost = count * INTERSECTION_COST;
        if (best_split == -1 || best_cost >= leaf_cost) {
            node->firstPrim = start;
            node->primCount = count;
            return node;
        }

        // Partition prim_indices_ by bin index relative to best_split
        auto mid_it = std::partition(
            prim_indices_.begin() + start,
            prim_indices_.begin() + end,
            [&](int idx) {
                double c = prim_centroids_[idx][axis];
                int b = static_cast<int>((c - min_c) * invExtent * BIN_COUNT);
                if (b < 0) b = 0;
                if (b >= BIN_COUNT) b = BIN_COUNT - 1;
                return b <= best_split;
            });

        int mid = static_cast<int>(mid_it - prim_indices_.begin());

        // Edge case: partition produced empty side
        int leftCount  = mid - start;
        int rightCount = end - mid;
        if (leftCount == 0 || rightCount == 0) {
            node->firstPrim = start;
            node->primCount = count;
            return node;
        }

        node->left  = BuildSah(start, mid);
        node->right = BuildSah(mid,   end);
        node->firstPrim = 0;
        node->primCount = 0;
        return node;
    }

    // Flatten build nodes into linear array of BvhNodeGPU
    int flatten(const BuildNode& bnode) {
        int idx = static_cast<int>(nodes_.size());
        nodes_.push_back({});
        BvhNodeGPU& out = nodes_.back();

        out.bbox = bnode.bounds;

        if (bnode.is_leaf()) {
            out.isLeaf = 1;
            out.left   = static_cast<uint32_t>(bnode.firstPrim);
            out.right  = static_cast<uint32_t>(bnode.primCount);
        } else {
            out.isLeaf = 0;
            int left_idx  = flatten(*bnode.left);
            int right_idx = flatten(*bnode.right);
            out.left  = static_cast<uint32_t>(left_idx);
            out.right = static_cast<uint32_t>(right_idx);
        }

        return idx;
    }
};

} // namespace rt::geom
