#include "scene/gltf_loader.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace m4w {

namespace {

const cgltf_accessor* findAttribute(const cgltf_primitive& prim, cgltf_attribute_type type) {
    for (cgltf_size i = 0; i < prim.attributes_count; ++i) {
        if (prim.attributes[i].type == type && prim.attributes[i].index == 0) {
            return prim.attributes[i].data;
        }
    }
    return nullptr;
}

void appendPrimitive(const cgltf_primitive& prim, const glm::mat4& world, MeshData& out) {
    if (prim.type != cgltf_primitive_type_triangles) {
        return;
    }
    const cgltf_accessor* pos = findAttribute(prim, cgltf_attribute_type_position);
    if (pos == nullptr) {
        return;
    }
    const cgltf_accessor* normal = findAttribute(prim, cgltf_attribute_type_normal);
    const cgltf_accessor* color = findAttribute(prim, cgltf_attribute_type_color);

    glm::vec4 base_color{1.0f};
    if (prim.material != nullptr && prim.material->has_pbr_metallic_roughness) {
        const float* f = prim.material->pbr_metallic_roughness.base_color_factor;
        base_color = glm::vec4(f[0], f[1], f[2], f[3]);
    }

    const glm::mat3 normal_mat = glm::mat3(world);  // válido sin escalado no uniforme
    const uint32_t vertex_base = static_cast<uint32_t>(out.vertices.size());

    for (cgltf_size v = 0; v < pos->count; ++v) {
        Vertex vert{};

        float p[3] = {0, 0, 0};
        cgltf_accessor_read_float(pos, v, p, 3);
        vert.pos = glm::vec3(world * glm::vec4(p[0], p[1], p[2], 1.0f));

        float n[3] = {0, 1, 0};
        if (normal != nullptr) {
            cgltf_accessor_read_float(normal, v, n, 3);
        }
        vert.normal = glm::normalize(normal_mat * glm::vec3(n[0], n[1], n[2]));

        float c[4] = {1, 1, 1, 1};
        if (color != nullptr) {
            cgltf_accessor_read_float(color, v, c, 4);
            if (cgltf_num_components(color->type) == 3) {
                c[3] = 1.0f;
            }
        }
        vert.color = glm::vec4(c[0], c[1], c[2], c[3]) * base_color;

        out.vertices.push_back(vert);
    }

    if (prim.indices != nullptr) {
        for (cgltf_size i = 0; i < prim.indices->count; ++i) {
            const uint32_t idx = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, i));
            out.indices.push_back(vertex_base + idx);
        }
    } else {
        for (cgltf_size i = 0; i < pos->count; ++i) {
            out.indices.push_back(vertex_base + static_cast<uint32_t>(i));
        }
    }
}

} // namespace

bool loadGltfMesh(const char* path, MeshData& out) {
    cgltf_options options{};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success) {
        SDL_Log("[gltf] no se pudo parsear %s (cgltf_result %d)", path, static_cast<int>(result));
        return false;
    }

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) {
        SDL_Log("[gltf] no se pudieron cargar los buffers de %s (cgltf_result %d)",
                path, static_cast<int>(result));
        cgltf_free(data);
        return false;
    }

    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        const cgltf_node& node = data->nodes[n];
        if (node.mesh == nullptr) {
            continue;
        }
        float m[16];
        cgltf_node_transform_world(&node, m);
        const glm::mat4 world = glm::make_mat4(m);

        for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p) {
            appendPrimitive(node.mesh->primitives[p], world, out);
        }
    }

    cgltf_free(data);

    if (out.vertices.empty()) {
        SDL_Log("[gltf] %s no contiene geometría de triángulos", path);
        return false;
    }
    return true;
}

} // namespace m4w
