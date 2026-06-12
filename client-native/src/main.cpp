#include <cstring>

#include <SDL3/SDL_main.h>

#include "platform/app.h"

int main(int argc, char** argv) {
    m4w::AppConfig config;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--selftest") == 0) {
            config.selftest = true;
        }
    }

    m4w::App app;
    return app.run(config);
}
