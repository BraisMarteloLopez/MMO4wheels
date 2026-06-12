# Cliente nativo (C++ + SDL3 GPU)

Cliente de escritorio de MMO4wheels con renderer propio sobre la SDL3 GPU API
(backend Vulkan, shaders GLSL 450 → SPIR-V). El build es autocontenido:
descarga y compila sus dependencias (SDL3, Dear ImGui, glslang) con versiones
fijadas — no hace falta el SDK de Vulkan.

## Requisitos

- CMake ≥ 3.24 y un compilador C++20.
- GPU con driver Vulkan (lo normal con drivers de NVIDIA/AMD/Intel al día).

**Linux (Ubuntu/Debian)** — instalar antes de configurar, para que SDL detecte
X11/Wayland (si configuraste sin ellas, borra `build/` y reconfigura):

```bash
sudo apt install build-essential cmake ninja-build \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
  libxkbcommon-dev libwayland-dev libegl1-mesa-dev libdrm-dev libgbm-dev \
  libasound2-dev libpulse-dev libudev-dev libdbus-1-dev libibus-1.0-dev \
  mesa-vulkan-drivers
```

**Windows**: Visual Studio 2022 con la carga de trabajo "Desarrollo de
escritorio con C++" (o MinGW/clang). Nada más.

## Compilar y ejecutar

Desde la raíz del repositorio:

```bash
cmake -S client-native -B client-native/build -DCMAKE_BUILD_TYPE=Release
cmake --build client-native/build          # añade --config Release en Visual Studio
./client-native/build/bin/mmo4wheels       # en Windows: client-native\build\bin\mmo4wheels.exe
```

La primera configuración descarga las dependencias y el primer build las
compila (varios minutos); los siguientes son incrementales.

**Qué deberías ver (M0′)**: ventana de 1280×720 con fondo azul oscuro, un
triángulo RGB y un panel "MMO4wheels — M0′" con los fps y el driver de GPU
(`vulkan`). `ESC` cierra.

## Diagnóstico

```bash
./client-native/build/bin/mmo4wheels --selftest
```

Arranca sin pantalla ni GPU (driver de vídeo dummy), imprime la versión de SDL
y los drivers de GPU compilados, y sale con código 0. Es lo que se ejecuta en
entornos sin display (CI, contenedores).
