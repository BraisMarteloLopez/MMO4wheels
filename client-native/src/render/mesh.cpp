#include "render/mesh.h"

#include <cstring>

namespace m4w {

GpuMesh uploadMesh(SDL_GPUDevice* device, const MeshData& data) {
    GpuMesh mesh;
    if (data.vertices.empty() || data.indices.empty()) {
        SDL_Log("[mesh] malla vacía");
        return mesh;
    }

    const Uint32 vbytes = static_cast<Uint32>(data.vertices.size() * sizeof(Vertex));
    const Uint32 ibytes = static_cast<Uint32>(data.indices.size() * sizeof(uint32_t));

    SDL_GPUBufferCreateInfo vinfo{};
    vinfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vinfo.size = vbytes;
    mesh.vertex_buffer = SDL_CreateGPUBuffer(device, &vinfo);

    SDL_GPUBufferCreateInfo iinfo{};
    iinfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    iinfo.size = ibytes;
    mesh.index_buffer = SDL_CreateGPUBuffer(device, &iinfo);

    if (mesh.vertex_buffer == nullptr || mesh.index_buffer == nullptr) {
        SDL_Log("[mesh] SDL_CreateGPUBuffer falló: %s", SDL_GetError());
        releaseMesh(device, mesh);
        return mesh;
    }

    SDL_GPUTransferBufferCreateInfo tinfo{};
    tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tinfo.size = vbytes + ibytes;
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &tinfo);
    if (transfer == nullptr) {
        SDL_Log("[mesh] SDL_CreateGPUTransferBuffer falló: %s", SDL_GetError());
        releaseMesh(device, mesh);
        return mesh;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(mapped, data.vertices.data(), vbytes);
    std::memcpy(static_cast<Uint8*>(mapped) + vbytes, data.indices.data(), ibytes);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src{};
    src.transfer_buffer = transfer;
    src.offset = 0;
    SDL_GPUBufferRegion dst{};
    dst.buffer = mesh.vertex_buffer;
    dst.offset = 0;
    dst.size = vbytes;
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);

    src.offset = vbytes;
    dst.buffer = mesh.index_buffer;
    dst.size = ibytes;
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);

    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    mesh.index_count = static_cast<Uint32>(data.indices.size());
    return mesh;
}

void releaseMesh(SDL_GPUDevice* device, GpuMesh& mesh) {
    if (mesh.vertex_buffer != nullptr) {
        SDL_ReleaseGPUBuffer(device, mesh.vertex_buffer);
        mesh.vertex_buffer = nullptr;
    }
    if (mesh.index_buffer != nullptr) {
        SDL_ReleaseGPUBuffer(device, mesh.index_buffer);
        mesh.index_buffer = nullptr;
    }
    mesh.index_count = 0;
}

} // namespace m4w
