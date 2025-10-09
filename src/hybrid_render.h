#pragma once

#include "bvh.h"
#include "vec3.h"
struct bvh_node_GPU {
    vec3 bbox_min;
    vec3 bbox_max;

    int left_or_first; // stores left child index, or primative index
    int count; // number of primatives (internal node has 0)
};

struct primitive_ref {
    int type;
    int index;
};

inline int flatten_bvh(const bvh_node* node,
                std::vector<bvh_node_GPU>& flat_nodes,
                std::vector<primitive_ref>& flat_prims)
{
    int current_index = flat_nodes.size();
    flat_nodes.push_back({}); // placeholder

    bvh_node_GPU gpu_node;
    gpu_node.bbox_min = node->bounding_box().min();
    gpu_node.bbox_max = node->bounding_box().max();

    if (!node->primitives.empty()) {
        // Leaf
        gpu_node.left_or_first = flat_prims.size();
        gpu_node.count = node->primitives.size();

        for (auto& obj : node->primitives) {
            primitive_ref ref;
            ref.type = obj->type_id(); // you’ll need to define this
            ref.index = obj->object_index(); // same
            flat_prims.push_back(ref);
        }
    } else {
        // Internal
        auto* left_bvh  = static_cast<bvh_node*>(node->left.get());
        auto* right_bvh = static_cast<bvh_node*>(node->right.get());

        int left_index  = flatten_bvh(left_bvh, flat_nodes, flat_prims);
        int right_index = flatten_bvh(right_bvh, flat_nodes, flat_prims);
        gpu_node.left_or_first = left_index;
        gpu_node.count = 0;  // mark as internal
    }

    flat_nodes[current_index] = gpu_node;
    return current_index;
}
