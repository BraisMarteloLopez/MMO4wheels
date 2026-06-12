#include "render/renderer.h"

#include <string>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

namespace m4w {

namespace {

constexpr SDL_GPUTextureFormat kDepthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

// std140; debe coincidir con VertUbo de mesh.vert / ground.vert
struct VertUbo {
    glm::mat4 mvp;
    glm::mat4 model;
};

// std140; debe coincidir con FragUbo de mesh.frag / ground.frag
struct FragUbo {
    glm::vec4 sun_dir;
    glm::vec4 sun_color;
    glm::vec4 ambient;
};

} // namespace

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

    mesh_pipeline_ = createScenePipeline("mesh.vert.spv", "mesh.frag.spv");
    ground_pipeline_ = createScenePipeline("ground.vert.spv", "ground.frag.spv");
    if (mesh_pipeline_ == nullptr || ground_pipeline_ == nullptr) {
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
    if (depth_texture_ != nullptr) {
        SDL_ReleaseGPUTexture(device_, depth_texture_);
        depth_texture_ = nullptr;
    }
    if (mesh_pipeline_ != nullptr) {
        SDL_ReleaseGPUGraphicsPipeline(device_, mesh_pipeline_);
        mesh_pipeline_ = nullptr;
    }
    if (ground_pipeline_ != nullptr) {
        SDL_ReleaseGPUGraphicsPipeline(device_, ground_pipeline_);
        ground_pipeline_ = nullptr;
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

void Renderer::newFrame() {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

bool Renderer::render(const Camera& camera, const SceneDraw& scene) {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device_);
    if (cmd == nullptr) {
        return false;
    }

    SDL_GPUTexture* swapchain = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window_, &swapchain, &width, &height)) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return false;
    }
    if (swapchain == nullptr || width == 0 || height == 0) {
        // Ventana minimizada: no hay backbuffer este frame.
        SDL_SubmitGPUCommandBuffer(cmd);
        return true;
    }

    if (!ensureDepthTexture(width, height)) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return false;
    }

    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const glm::mat4 view_proj = camera.proj(aspect) * camera.view();

    FragUbo frag_ubo{};
    frag_ubo.sun_dir = glm::vec4(glm::normalize(scene.light.sun_dir), 0.0f);
    frag_ubo.sun_color = glm::vec4(scene.light.sun_color, scene.light.sun_intensity);
    frag_ubo.ambient = glm::vec4(scene.light.ambient, 0.0f);

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd);

    SDL_GPUColorTargetInfo color_target{};
    color_target.texture = swapchain;
    color_target.clear_color = SDL_FColor{0.07f, 0.08f, 0.12f, 1.0f};
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depth_target{};
    depth_target.texture = depth_texture_;
    depth_target.clear_depth = 1.0f;
    depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth_target.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth_target.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth_target.cycle = true;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &color_target, 1, &depth_target);

    SDL_PushGPUFragmentUniformData(cmd, 0, &frag_ubo, sizeof(frag_ubo));

    auto drawMesh = [&](const GpuMesh& mesh, const glm::mat4& model) {
        VertUbo vert_ubo{};
        vert_ubo.mvp = view_proj * model;
        vert_ubo.model = model;
        SDL_PushGPUVertexUniformData(cmd, 0, &vert_ubo, sizeof(vert_ubo));

        SDL_GPUBufferBinding vbind{};
        vbind.buffer = mesh.vertex_buffer;
        SDL_BindGPUVertexBuffers(pass, 0, &vbind, 1);

        SDL_GPUBufferBinding ibind{};
        ibind.buffer = mesh.index_buffer;
        SDL_BindGPUIndexBuffer(pass, &ibind, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_DrawGPUIndexedPrimitives(pass, mesh.index_count, 1, 0, 0, 0);
    };

    if (scene.ground != nullptr && scene.ground->valid()) {
        SDL_BindGPUGraphicsPipeline(pass, ground_pipeline_);
        drawMesh(*scene.ground, glm::mat4(1.0f));
    }

    SDL_BindGPUGraphicsPipeline(pass, mesh_pipeline_);
    for (const DrawItem& item : scene.items) {
        if (item.mesh != nullptr && item.mesh->valid()) {
            drawMesh(*item.mesh, item.model);
        }
    }

    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd, pass);
    SDL_EndGPURenderPass(pass);

    return SDL_SubmitGPUCommandBuffer(cmd);
}

bool Renderer::ensureDepthTexture(Uint32 width, Uint32 height) {
    if (depth_texture_ != nullptr && depth_width_ == width && depth_height_ == height) {
        return true;
    }
    if (depth_texture_ != nullptr) {
        SDL_ReleaseGPUTexture(device_, depth_texture_);
        depth_texture_ = nullptr;
    }

    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = kDepthFormat;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;

    depth_texture_ = SDL_CreateGPUTexture(device_, &info);
    if (depth_texture_ == nullptr) {
        SDL_Log("[render] no se pudo crear la textura de profundidad: %s", SDL_GetError());
        return false;
    }
    depth_width_ = width;
    depth_height_ = height;
    return true;
}

SDL_GPUGraphicsPipeline* Renderer::createScenePipeline(const char* vert, const char* frag) {
    SDL_GPUShader* vshader = loadShader(vert, SDL_GPU_SHADERSTAGE_VERTEX);
    SDL_GPUShader* fshader = loadShader(frag, SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (vshader == nullptr || fshader == nullptr) {
        return nullptr;
    }

    SDL_GPUVertexBufferDescription vbuf_desc{};
    vbuf_desc.slot = 0;
    vbuf_desc.pitch = sizeof(Vertex);
    vbuf_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[3]{};
    attrs[0].location = 0;
    attrs[0].buffer_slot = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[0].offset = offsetof(Vertex, pos);
    attrs[1].location = 1;
    attrs[1].buffer_slot = 0;
    attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[1].offset = offsetof(Vertex, normal);
    attrs[2].location = 2;
    attrs[2].buffer_slot = 0;
    attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attrs[2].offset = offsetof(Vertex, color);

    SDL_GPUColorTargetDescription color_desc{};
    color_desc.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vshader;
    info.fragment_shader = fshader;
    info.vertex_input_state.vertex_buffer_descriptions = &vbuf_desc;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attrs;
    info.vertex_input_state.num_vertex_attributes = 3;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    // Culling desactivado hasta confirmar visualmente la orientación de las
    // caras (M2' lo activará junto al trabajo del look).
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    info.target_info.color_target_descriptions = &color_desc;
    info.target_info.num_color_targets = 1;
    info.target_info.has_depth_stencil_target = true;
    info.target_info.depth_stencil_format = kDepthFormat;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &info);

    SDL_ReleaseGPUShader(device_, vshader);
    SDL_ReleaseGPUShader(device_, fshader);

    if (pipeline == nullptr) {
        SDL_Log("[render] SDL_CreateGPUGraphicsPipeline falló (%s + %s): %s",
                vert, frag, SDL_GetError());
    }
    return pipeline;
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
    info.num_uniform_buffers = 1;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device_, &info);
    SDL_free(code);

    if (shader == nullptr) {
        SDL_Log("[render] SDL_CreateGPUShader falló (%s): %s", filename, SDL_GetError());
    }
    return shader;
}

} // namespace m4w
