#include "platform/app.h"

namespace m4w {

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

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
        const float dt = static_cast<float>(now - last) / 1e9f;
        last = now;

        const float fps = dt > 0.0f ? 1.0f / dt : 0.0f;
        fps_smooth = fps_smooth == 0.0f ? fps : fps_smooth + (fps - fps_smooth) * 0.05f;

        FrameStats stats{};
        stats.fps = fps_smooth;
        stats.frame_ms = dt * 1000.0f;

        if (!renderer_.drawFrame(stats)) {
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

    SDL_Log("[app] listo (driver de vídeo: %s, driver de GPU: %s)",
            SDL_GetCurrentVideoDriver(), renderer_.driverName());
    return true;
}

void App::shutdown() {
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

    SDL_Quit();
    SDL_Log("[selftest] ok");
    return 0;
}

} // namespace m4w
