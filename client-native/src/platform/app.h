#pragma once

#include <SDL3/SDL.h>

#include "render/camera.h"
#include "render/renderer.h"
#include "sim/car.h"

namespace m4w {

struct AppConfig {
    // Valida binario, física y carga de assets sin GPU ni pantalla
    // (driver de vídeo dummy): para CI/contenedores.
    bool selftest = false;
};

class App {
public:
    int run(const AppConfig& config);

private:
    bool init();
    void shutdown();
    void processEvents();
    CarInput sampleInput() const;
    void updateCamera(float dt);
    void buildUi(float fps, float frame_ms);
    int selftest();

    SDL_Window* window_ = nullptr;
    Renderer renderer_;
    bool quit_ = false;

    // simulación local (M1'; en M3' pasa a mandar el servidor)
    CarState car_;
    CarParams params_ = defaultCarParams();

    // escena
    GpuMesh car_mesh_;
    GpuMesh ground_mesh_;
    LightParams light_;

    // cámara aérea con seguimiento
    Camera camera_;
    float cam_yaw_ = 0.0f;
    bool cam_follow_heading_ = true;
    float cam_height_ = 13.0f;
    float cam_distance_ = 9.5f;
    float cam_lookahead_ = 3.0f;
};

} // namespace m4w
