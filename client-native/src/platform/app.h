#pragma once

#include <SDL3/SDL.h>

#include "render/renderer.h"

namespace m4w {

struct AppConfig {
    // Arranca sin GPU ni pantalla (driver de vídeo dummy) y sale: valida que
    // el binario y SDL funcionan en entornos sin display (CI, contenedores).
    bool selftest = false;
};

class App {
public:
    int run(const AppConfig& config);

private:
    bool init();
    void shutdown();
    void processEvents();
    int selftest();

    SDL_Window* window_ = nullptr;
    Renderer renderer_;
    bool quit_ = false;
};

} // namespace m4w
