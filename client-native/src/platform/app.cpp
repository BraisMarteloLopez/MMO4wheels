#include "platform/app.h"

#include <cmath>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "generated/constants.h"
#include "scene/gltf_loader.h"
#include "scene/procedural.h"

namespace m4w {

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr float kMaxDt = 1.0f / 30.0f;
constexpr float kGroundHalfExtent = 300.0f;
constexpr float kPi = 3.14159265358979323846f;  // M_PI no es portable (MSVC)

// diferencia angular más corta, en (-pi, pi]
float angleDelta(float from, float to) {
    float d = std::fmod(to - from, 2.0f * kPi);
    if (d > kPi) d -= 2.0f * kPi;
    if (d < -kPi) d += 2.0f * kPi;
    return d;
}

std::string assetPath(const char* relative) {
    return std::string(SDL_GetBasePath()) + relative;
}

glm::mat4 carModelMatrix(const CarState& car) {
    // sim (x, y) -> mundo (x, 0, y); heading 0 = +X, crece hacia +Z
    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(car.x, 0.0f, car.y));
    return glm::rotate(m, -car.heading, glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace

int App::run(const AppConfig& config) {
    SDL_SetAppMetadata("MMO4wheels", "0.1.0", "dev.mmo4wheels.client");

    if (config.selftest) {
        return selftest();
    }

    if (!init()) {
        shutdown();
        return 1;
    }

    Uint64 last = SDL_GetTicksNS();
    float fps_smooth = 0.0f;

    while (!quit_) {
        processEvents();

        const Uint64 now = SDL_GetTicksNS();
        float dt = static_cast<float>(now - last) / 1e9f;
        last = now;
        const float fps = dt > 0.0f ? 1.0f / dt : 0.0f;
        fps_smooth = fps_smooth == 0.0f ? fps : fps_smooth + (fps - fps_smooth) * 0.05f;
        dt = std::min(dt, kMaxDt);

        stepCar(car_, sampleInput(), dt, params_);
        updateCamera(dt);

        renderer_.newFrame();
        buildUi(fps_smooth, dt * 1000.0f);

        const DrawItem items[] = {{&car_mesh_, carModelMatrix(car_)}};
        SceneDraw scene{};
        scene.ground = &ground_mesh_;
        scene.items = items;
        scene.light = light_;

        if (!renderer_.render(camera_, scene)) {
            SDL_Log("[app] error de render: %s", SDL_GetError());
            quit_ = true;
        }
    }

    shutdown();
    return 0;
}

bool App::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("[app] SDL_Init falló: %s", SDL_GetError());
        return false;
    }

    window_ = SDL_CreateWindow("MMO4wheels", kWindowWidth, kWindowHeight, SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        SDL_Log("[app] SDL_CreateWindow falló: %s", SDL_GetError());
        return false;
    }

    if (!renderer_.init(window_)) {
        return false;
    }

    MeshData car_data;
    if (!loadGltfMesh(assetPath("assets/models/car.gltf").c_str(), car_data)) {
        return false;
    }
    car_mesh_ = uploadMesh(renderer_.device(), car_data);

    MeshData ground_data = makeGroundPlane(kGroundHalfExtent);
    ground_mesh_ = uploadMesh(renderer_.device(), ground_data);

    if (!car_mesh_.valid() || !ground_mesh_.valid()) {
        return false;
    }

    SDL_Log("[app] listo (vídeo: %s, gpu: %s, coche: %u índices)",
            SDL_GetCurrentVideoDriver(), renderer_.driverName(), car_mesh_.index_count);
    return true;
}

void App::shutdown() {
    if (renderer_.device() != nullptr) {
        SDL_WaitForGPUIdle(renderer_.device());
        releaseMesh(renderer_.device(), car_mesh_);
        releaseMesh(renderer_.device(), ground_mesh_);
    }
    renderer_.shutdown();
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void App::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        renderer_.processEvent(event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit_ = true;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    quit_ = true;
                }
                break;
            default:
                break;
        }
    }
}

CarInput App::sampleInput() const {
    const bool* keys = SDL_GetKeyboardState(nullptr);
    CarInput input{};

    if (ImGui::GetIO().WantCaptureKeyboard) {
        return input;
    }

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) input.throttle += 1.0f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) input.throttle -= 1.0f;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) input.steer += 1.0f;
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) input.steer -= 1.0f;
    input.brake = keys[SDL_SCANCODE_SPACE];

    return input;
}

void App::updateCamera(float dt) {
    if (cam_follow_heading_) {
        const float k = 1.0f - std::exp(-5.0f * dt);
        cam_yaw_ += angleDelta(cam_yaw_, car_.heading) * k;
    }

    const glm::vec3 car_pos(car_.x, 0.0f, car_.y);
    const glm::vec3 yaw_dir(std::cos(cam_yaw_), 0.0f, std::sin(cam_yaw_));
    const glm::vec3 heading_dir(std::cos(car_.heading), 0.0f, std::sin(car_.heading));

    camera_.position = car_pos - yaw_dir * cam_distance_ + glm::vec3(0.0f, cam_height_, 0.0f);
    camera_.target = car_pos + heading_dir * cam_lookahead_ + glm::vec3(0.0f, 0.8f, 0.0f);
}

void App::buildUi(float fps, float frame_ms) {
    const float speed = std::sqrt(car_.vx * car_.vx + car_.vy * car_.vy);

    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("MMO4wheels — M1'", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("fps: %.0f (%.2f ms) · gpu: %s", fps, frame_ms, renderer_.driverName());
    ImGui::Text("velocidad: %.0f km/h", speed * 3.6f);
    ImGui::Text("pos: (%.1f, %.1f) · rumbo: %.2f rad", car_.x, car_.y, car_.heading);
    ImGui::TextDisabled("WASD/flechas conducir · espacio freno · ESC salir");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(12.0f, 150.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Ajustes (tuneo en vivo)")) {
        if (ImGui::CollapsingHeader("Física", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("aceleración", &params_.accel, 5.0f, 60.0f, "%.1f m/s²");
            ImGui::SliderFloat("acel. marcha atrás", &params_.reverse_accel, 2.0f, 30.0f, "%.1f m/s²");
            ImGui::SliderFloat("vel. máxima", &params_.max_speed, 10.0f, 80.0f, "%.1f m/s");
            ImGui::SliderFloat("frenada", &params_.brake_decel, 10.0f, 80.0f, "%.1f m/s²");
            ImGui::SliderFloat("rodadura", &params_.roll_decel, 0.0f, 10.0f, "%.2f m/s²");
            ImGui::SliderFloat("arrastre q", &params_.drag_q, 0.0f, 0.08f, "%.4f");
            ImGui::SliderFloat("giro", &params_.steer_rate, 0.5f, 6.0f, "%.2f rad/s");
            ImGui::SliderFloat("vel. ref. giro", &params_.steer_ref_speed, 1.0f, 20.0f, "%.1f m/s");
            ImGui::SliderFloat("agarre", &params_.grip, 0.5f, 20.0f, "%.1f /s");
            if (ImGui::Button("restaurar")) {
                params_ = defaultCarParams();
            }
            ImGui::SameLine();
            if (ImGui::Button("volcar al log")) {
                SDL_Log("[tuneo] ACCEL=%.2f REVERSE_ACCEL=%.2f MAX_SPEED=%.2f BRAKE_DECEL=%.2f "
                        "ROLL_DECEL=%.2f DRAG_Q=%.4f STEER_RATE=%.2f STEER_REF_SPEED=%.2f GRIP=%.2f",
                        params_.accel, params_.reverse_accel, params_.max_speed, params_.brake_decel,
                        params_.roll_decel, params_.drag_q, params_.steer_rate,
                        params_.steer_ref_speed, params_.grip);
            }
        }
        if (ImGui::CollapsingHeader("Cámara")) {
            ImGui::Checkbox("seguir rumbo", &cam_follow_heading_);
            ImGui::SliderFloat("altura", &cam_height_, 4.0f, 40.0f, "%.1f m");
            ImGui::SliderFloat("distancia", &cam_distance_, 2.0f, 30.0f, "%.1f m");
            ImGui::SliderFloat("anticipación", &cam_lookahead_, 0.0f, 12.0f, "%.1f m");
            ImGui::SliderFloat("fov", &camera_.fov_deg, 30.0f, 80.0f, "%.0f°");
        }
    }
    ImGui::End();
}

int App::selftest() {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("[selftest] SDL_Init falló: %s", SDL_GetError());
        return 1;
    }

    SDL_Log("[selftest] SDL %d.%d.%d, driver de vídeo: %s",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION,
            SDL_GetCurrentVideoDriver());

    const int n = SDL_GetNumGPUDrivers();
    SDL_Log("[selftest] drivers de GPU compilados (%d):", n);
    for (int i = 0; i < n; ++i) {
        SDL_Log("[selftest]   - %s", SDL_GetGPUDriver(i));
    }

    // física: 10 s a fondo en línea recta, luego 1 s girando
    CarState car{};
    const CarParams params = defaultCarParams();
    CarInput input{};
    input.throttle = 1.0f;
    for (int i = 0; i < 600; ++i) {
        stepCar(car, input, 1.0f / 60.0f, params);
    }
    const float x_recta = car.x;
    const float v = std::sqrt(car.vx * car.vx + car.vy * car.vy);
    if (!std::isfinite(x_recta) || x_recta < 150.0f || std::abs(car.y) > 0.01f || v > params.max_speed + 0.01f) {
        SDL_Log("[selftest] FÍSICA MAL: x=%.1f y=%.3f v=%.1f", x_recta, car.y, v);
        return 1;
    }
    input.steer = 1.0f;
    for (int i = 0; i < 60; ++i) {
        stepCar(car, input, 1.0f / 60.0f, params);
    }
    if (std::abs(car.heading) < 0.1f) {
        SDL_Log("[selftest] FÍSICA MAL: no gira (heading=%.3f)", car.heading);
        return 1;
    }
    // x_recta debe coincidir con shared/scripts/physics-smoke.ts (paridad TS<->C++)
    SDL_Log("[selftest] física ok (recta: %.2f m a %.2f m/s; giro: %.2f rad)", x_recta, v, car.heading);

    // carga de assets: el glTF del coche
    MeshData mesh;
    if (!loadGltfMesh(assetPath("assets/models/car.gltf").c_str(), mesh)) {
        SDL_Log("[selftest] FALLO al cargar assets/models/car.gltf");
        return 1;
    }
    SDL_Log("[selftest] glTF ok (%zu vértices, %zu índices)", mesh.vertices.size(), mesh.indices.size());

    SDL_Quit();
    SDL_Log("[selftest] ok");
    return 0;
}

} // namespace m4w
