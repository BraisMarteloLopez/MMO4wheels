#include "render/renderer.h"

#include <string>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

namespace m4w {

bool Renderer::init(SDL_Window* window) {
    window_ = window;

    device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, /*debug_mode=*/true, /*name=*/nullptr);
    if (device_ == nullptr) {
        SDL_Log("[render] SDL_CreateGPUDevice falló: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device_, window_)) {
        SDL_Log("[render] SDL_ClaimWindowForGPUDevice falló: %s", SDL_GetError());
        return false;
    }

    if (!createTrianglePipeline()) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    if (!ImGui_ImplSDL3_InitForOther(window_)) {
        SDL_Log("[render] ImGui_ImplSDL3_InitForOther falló");
        return false;
    }
    ImGui_ImplSDLGPU3_InitInfo imgui_info{};
    imgui_info.Device = device_;
    imgui_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device_, window_);
    imgui_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    if (!ImGui_ImplSDLGPU3_Init(&imgui_info)) {
        SDL_Log("[render] ImGui_ImplSDLGPU3_Init falló");
        return false;
    }
    imgui_ready_ = true;

    return true;
}

void Renderer::shutdown() {
    if (device_ != nullptr) {
        SDL_WaitForGPUIdle(device_);
    }
    if (imgui_ready_) {
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        imgui_ready_ = false;
    }
    if (triangle_pipeline_ != nullptr) {
        SDL_ReleaseGPUGraphicsPipeline(device_, triangle_pipeline_);
        triangle_pipeline_ = nullptr;
    }
    if (device_ != nullptr) {
        if (window_ != nullptr) {
            SDL_ReleaseWindowFromGPUDevice(device_, window_);
        }
        SDL_DestroyGPUDevice(device_);
        device_ = nullptr;
    }
}

void Renderer::processEvent(const SDL_Event& event) {
    if (imgui_ready_) {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }
}

const char* Renderer::driverName() const {
    return device_ != nullptr ? SDL_GetGPUDeviceDriver(device_) : "(sin device)";
}

bool Renderer::drawFrame(const FrameStats& stats) {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("MMO4wheels — M0'", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("fps: %.0f (%.2f ms)", stats.fps, stats.frame_ms);
    ImGui::Text("gpu: %s", driverName());
    ImGui::TextDisabled("ESC para salir");
    ImGui::End();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device_);
    if (cmd == nullptr) {
        return false;
    }

    SDL_GPUTexture* swapchain = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window_, &swapchain, nullptr, nullptr)) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return false;
    }
    if (swapchain == nullptr) {
        // Ventana minimizada: no hay backbuffer este frame.
        SDL_SubmitGPUCommandBuffer(cmd);
        return true;
    }

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd);

    SDL_GPUColorTargetInfo color_target{};
    color_target.texture = swapchain;
    color_target.clear_color = SDL_FColor{0.07f, 0.08f, 0.14f, 1.0f};
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &color_target, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(pass, triangle_pipeline_);
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd, pass);
    SDL_EndGPURenderPass(pass);

    return SDL_SubmitGPUCommandBuffer(cmd);
}

bool Renderer::createTrianglePipeline() {
    SDL_GPUShader* vert = loadShader("triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX);
    SDL_GPUShader* frag = loadShader("triangle.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (vert == nullptr || frag == nullptr) {
        return false;
    }

    SDL_GPUColorTargetDescription color_desc{};
    color_desc.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vert;
    info.fragment_shader = frag;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.target_info.color_target_descriptions = &color_desc;
    info.target_info.num_color_targets = 1;

    triangle_pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &info);

    SDL_ReleaseGPUShader(device_, vert);
    SDL_ReleaseGPUShader(device_, frag);

    if (triangle_pipeline_ == nullptr) {
        SDL_Log("[render] SDL_CreateGPUGraphicsPipeline falló: %s", SDL_GetError());
        return false;
    }
    return true;
}

SDL_GPUShader* Renderer::loadShader(const char* filename, SDL_GPUShaderStage stage) {
    const std::string path = std::string(SDL_GetBasePath()) + "shaders/" + filename;

    size_t size = 0;
    void* code = SDL_LoadFile(path.c_str(), &size);
    if (code == nullptr) {
        SDL_Log("[render] no se pudo leer el shader %s: %s", path.c_str(), SDL_GetError());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};
    info.code = static_cast<const Uint8*>(code);
    info.code_size = size;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = stage;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device_, &info);
    SDL_free(code);

    if (shader == nullptr) {
        SDL_Log("[render] SDL_CreateGPUShader falló (%s): %s", filename, SDL_GetError());
    }
    return shader;
}

} // namespace m4w
