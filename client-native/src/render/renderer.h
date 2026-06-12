#pragma once

#include <SDL3/SDL.h>

namespace m4w {

struct FrameStats {
    float fps = 0.0f;
    float frame_ms = 0.0f;
};

// Renderer propio sobre la SDL3 GPU API (backend Vulkan, shaders SPIR-V).
// M0': swapchain con clear, un pipeline gráfico (triángulo) y overlay ImGui.
class Renderer {
public:
    bool init(SDL_Window* window);
    void shutdown();

    void processEvent(const SDL_Event& event);
    bool drawFrame(const FrameStats& stats);

    const char* driverName() const;

private:
    bool createTrianglePipeline();
    SDL_GPUShader* loadShader(const char* filename, SDL_GPUShaderStage stage);

    SDL_Window* window_ = nullptr;
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUGraphicsPipeline* triangle_pipeline_ = nullptr;
    bool imgui_ready_ = false;
};

} // namespace m4w
