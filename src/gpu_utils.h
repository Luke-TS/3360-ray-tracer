#pragma once

#include "bvh.h"
#include "vec3.h"
struct BvhNodeGPU {
    Vec3 bbox_min;
    Vec3 bbox_max;

    int left_or_first; // stores left child index, or primative index
    int count; // number of primatives (internal node has 0)
};

struct PrimativeRef {
    int type;
    int index;
};

/**
* Converts bvh tree to flat vectors for purposes of CUDA programming.
* flat_nodes contains bvh_node_GPU struct containing bbox information and child node or primative information
* flat_prims containts primitive_ref struct which contains an enum type (triangle, square, circle, etc) and the object index
*/
inline int flatten_bvh(const BvhNode* node,
                std::vector<BvhNodeGPU>& flat_nodes,
                std::vector<PrimativeRef>& flat_prims)
{
    int current_index = flat_nodes.size();
    flat_nodes.push_back({}); // placeholder

    BvhNodeGPU gpu_node;
    gpu_node.bbox_min = node->bounding_box().min();
    gpu_node.bbox_max = node->bounding_box().max();

    if (!node->primitives.empty()) {
        // Leaf
        gpu_node.left_or_first = flat_prims.size();
        gpu_node.count = node->primitives.size();

        for (auto& obj : node->primitives) {
            PrimativeRef ref;
            ref.type = obj->type_id(); // you’ll need to define this
            ref.index = obj->object_index(); // same
            flat_prims.push_back(ref);
        }
    } else {
        // Internal
        auto* left_bvh  = static_cast<BvhNode*>(node->left.get());
        auto* right_bvh = static_cast<BvhNode*>(node->right.get());

        int left_index  = flatten_bvh(left_bvh, flat_nodes, flat_prims);
        int right_index = flatten_bvh(right_bvh, flat_nodes, flat_prims);
        gpu_node.left_or_first = left_index;
        gpu_node.count = 0;  // mark as internal
    }

    flat_nodes[current_index] = gpu_node;
    return current_index;
}
