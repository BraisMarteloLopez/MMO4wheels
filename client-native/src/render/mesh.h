#pragma once

#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace m4w {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 color;
};

// Malla en CPU, lista para subir a GPU.
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct GpuMesh {
    SDL_GPUBuffer* vertex_buffer = nullptr;
    SDL_GPUBuffer* index_buffer = nullptr;
    Uint32 index_count = 0;

    bool valid() const { return vertex_buffer != nullptr && index_buffer != nullptr; }
};

// Sube la malla a GPU (transfer buffer + copy pass síncrono).
GpuMesh uploadMesh(SDL_GPUDevice* device, const MeshData& data);
void releaseMesh(SDL_GPUDevice* device, GpuMesh& mesh);

} // namespace m4w
