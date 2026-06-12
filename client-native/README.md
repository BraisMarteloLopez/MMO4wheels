# Cliente nativo (C++ + SDL3 GPU) — Windows 10/11

Cliente de escritorio de MMO4wheels con renderer propio sobre la SDL3 GPU API
(backend Vulkan, shaders GLSL 450 → SPIR-V). **Plataforma objetivo: Windows
10/11 x64.** El build es autocontenido: descarga y compila sus dependencias
(SDL3, Dear ImGui, glslang) con versiones fijadas — no hace falta instalar el
SDK de Vulkan ni ninguna librería.

## Requisitos (Windows 10/11)

1. **Visual Studio 2022** (la edición Community gratuita vale):
   <https://visualstudio.microsoft.com/es/downloads/>
   - En el instalador, marca la carga de trabajo **«Desarrollo para el
     escritorio con C++»**. Eso incluye el compilador MSVC, CMake y Ninja —
     no necesitas instalar nada más.
2. **Git para Windows**: <https://git-scm.com/download/win> (si no lo tienes ya).
3. **Drivers de GPU al día** (NVIDIA/AMD/Intel). El runtime de Vulkan viene
   incluido en los drivers modernos; no se instala aparte.

## Compilar y ejecutar

Abre **«Developer PowerShell for VS 2022»** (búscalo en el menú Inicio — es
importante usar esta consola y no el PowerShell normal, porque trae CMake y el
compilador en el PATH) y ejecuta:

```powershell
git clone https://github.com/BraisMarteloLopez/MMO4wheels.git
cd MMO4wheels
git checkout claude/charming-shannon-2m78mr

cmake -S client-native -B client-native\build
cmake --build client-native\build --config Release

.\client-native\build\bin\mmo4wheels.exe
```

La primera configuración descarga las dependencias y el primer build las
compila (varios minutos); los siguientes builds son incrementales y rápidos.

**Qué deberías ver (M0′)**: ventana de 1280×720 con fondo azul oscuro, un
triángulo RGB y un panel «MMO4wheels — M0′» con los fps y el driver de GPU
(`vulkan`). `ESC` cierra.

## Problemas típicos

| Síntoma | Causa y solución |
|---|---|
| `cmake` no se reconoce como comando | Estás en un PowerShell normal. Abre «Developer PowerShell for VS 2022» (o instala CMake suelto y añádelo al PATH). |
| `SDL_CreateGPUDevice falló` al ejecutar | El driver de la GPU no expone Vulkan: actualiza los drivers desde la web de NVIDIA/AMD/Intel (no desde Windows Update). |
| Ejecutas por Escritorio Remoto / VM sin GPU | No habrá dispositivo Vulkan. Prueba `mmo4wheels.exe --selftest` para validar el resto. |
| El antivirus retiene el exe recién compilado | Falso positivo habitual con binarios sin firmar; añade una exclusión para la carpeta del proyecto. |

## Diagnóstico

```powershell
.\client-native\build\bin\mmo4wheels.exe --selftest
```

Arranca sin pantalla ni GPU (driver de vídeo dummy), imprime la versión de SDL
y los drivers de GPU compilados, y sale con código 0.

## Linux (entorno de verificación del agente)

El cliente también compila en Linux, que se usa como entorno de build y
selftest headless en remoto (no es plataforma de juego objetivo). Dependencias
de desarrollo en Ubuntu/Debian antes de configurar:

```bash
sudo apt install build-essential cmake ninja-build \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
  libxkbcommon-dev libwayland-dev libegl1-mesa-dev libdrm-dev libgbm-dev \
  libasound2-dev libpulse-dev libudev-dev libdbus-1-dev libibus-1.0-dev \
  mesa-vulkan-drivers
```
