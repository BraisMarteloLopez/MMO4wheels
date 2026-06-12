#pragma once

#include <span>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "render/camera.h"
#include "render/mesh.h"

namespace m4w {

struct DrawItem {
    const GpuMesh* mesh = nullptr;
    glm::mat4 model{1.0f};
};

struct LightParams {
    glm::vec3 sun_dir{0.45f, 0.8f, 0.35f};  // dirección HACIA la luz
    glm::vec3 sun_color{1.0f, 0.96f, 0.88f};
    float sun_intensity = 1.0f;
    glm::vec3 ambient{0.30f, 0.32f, 0.38f};
};

struct SceneDraw {
    const GpuMesh* ground = nullptr;
    std::span<const DrawItem> items;
    LightParams light;
};

// Renderer propio sobre la SDL3 GPU API (backend Vulkan, shaders SPIR-V).
// M1': pase forward con profundidad — suelo con rejilla + mallas con luz
// direccional — y overlay de ImGui en el mismo pass.
class Renderer {
public:
    bool init(SDL_Window* window);
    void shutdown();

    void processEvent(const SDL_Event& event);

    // Inicia el frame de ImGui; la app construye su UI entre newFrame() y render().
    void newFrame();
    bool render(const Camera& camera, const SceneDraw& scene);

    SDL_GPUDevice* device() const { return device_; }
    const char* driverName() const;

private:
    SDL_GPUShader* loadShader(const char* filename, SDL_GPUShaderStage stage);
    SDL_GPUGraphicsPipeline* createScenePipeline(const char* vert, const char* frag);
    bool ensureDepthTexture(Uint32 width, Uint32 height);

    SDL_Window* window_ = nullptr;
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUGraphicsPipeline* mesh_pipeline_ = nullptr;
    SDL_GPUGraphicsPipeline* ground_pipeline_ = nullptr;
    SDL_GPUTexture* depth_texture_ = nullptr;
    Uint32 depth_width_ = 0;
    Uint32 depth_height_ = 0;
    bool imgui_ready_ = false;
};

} // namespace m4w
