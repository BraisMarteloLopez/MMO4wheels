# Arquitectura

## Vista general

```
┌─────────── Cliente nativo (C++ + SDL3 GPU) ───────────┐      ┌────────────── Servidor (Node.js) ─────────────┐
│                                                        │      │  Colyseus + TypeScript                         │
│  Plataforma (SDL3): ventana, input, audio, GPU device  │  WS  │                                                │
│  Renderer propio (SDL3 GPU API, GLSL 450 → SPIR-V):    │◄────►│  · WorldRoom (sala autoritativa)               │
│   · cámara aérea, pipelines y passes explícitos        │      │  · Simulación 2D en planta a tick fijo (20 Hz) │
│   · luz direccional, niebla, tonemap + bloom           │      │  · stepCar() + colisiones + zonas              │
│   · sombras simples (shadow map / blob)                │      │  · Validación de inputs                        │
│  Escena: cgltf (glTF), stb_image, GLM                  │      │  · Estado: Colyseus Schema                     │
│  Mundo: snapshots interpolados (~100 ms)               │      │  · Persistencia (POC: JSON → interfaz Storage) │
│  Red: SDK nativo Colyseus (spike) / WS propio          │      └────────────────────────────────────────────────┘
│  Debug: Dear ImGui (ping, tick, overlays)              │                              ▲
└────────────────────────────────────────────────────────┘                              │ colisión 2D y zonas
                         ▲                                                              │ (nodos col_*/zone_*)
                         │ mallas y materiales                                          │
                  ┌──────┴────────────────────────────────────────────────────────────┐│
                  │  assets/levels/*.gltf — exportado desde Blender                    ├┘
                  │  (única fuente de verdad del nivel)                                │
                  └────────────────────────────────────────────────────────────────────┘
```

Principio rector intacto: **el servidor simula, el cliente presenta**. La simulación es plana (x, y, rumbo — ver D9); el 3D es presentación.

## Servidor

Sin cambios de fondo respecto al M0 verificado:

- **Colyseus `WorldRoom`**: una sala = el mundo. Tick fijo a 20 Hz (`setSimulationInterval`), consume inputs, ejecuta `stepCar()` (TypeScript, en `shared/`), resuelve colisiones 2D y evalúa zonas.
- **Estado como Schema**: difusión automática de cambios. Si el spike de red del cliente nativo (D10) descarta el schema binario, el estado pasa a difundirse como mensajes msgpack/JSON — la lógica de sala no cambia.
- **Validación**: el cliente envía *intenciones* (`input { seq, throttle, steer, brake }`), nunca posiciones.
- **Niveles**: al arrancar, la sala carga el glTF del nivel (es JSON + buffers), filtra nodos por convención de nombres y construye: polígonos/AABBs de colisión proyectados al plano, zonas de interacción, puntos de spawn.
- **Persistencia**: interfaz `Storage` (POC: JSON en disco; sustituible por SQLite/Postgres).
- **Identidad POC**: id anónimo persistido en el cliente; autenticación real queda fuera.

## Cliente nativo

Capas, de abajo arriba:

1. **Plataforma — SDL3**: ventana, eventos/input, audio y la creación del *GPU device*. Es la única dependencia "grande"; sustituye a la pareja GLFW+OpenGL del enfoque clásico.
2. **Renderer propio — sobre la SDL3 GPU API**: command buffers, render passes y pipelines explícitos escritos por nosotros. Pase principal con luz direccional y niebla por distancia; sombras simples (shadow map básico, con *blob shadows* como salida digna si se enquista); post-procesado mínimo (tonemapping + bloom ligero) sobre render targets. Shaders en GLSL `#version 450` compilados a **SPIR-V** como paso del build: CMake compila **glslang** desde fuentes como herramienta del propio proyecto (no hace falta tener instalado el SDK de Vulkan ni `glslc`). Durante el POC se fuerza el **backend Vulkan** en Windows y Linux: un único formato de shader; DX12/Metal vía SDL_shadercross cuando interese (macOS incluido).
3. **Escena/assets**: carga de glTF con **cgltf**, texturas con **stb_image**, matemáticas con **GLM**. Materiales de color plano (low-poly): el "material" del POC es deliberadamente mínimo.
4. **Mundo**: réplica local del estado del servidor. Entidades renderizadas con **interpolación** entre los dos últimos snapshots (~100 ms de retraso de presentación). Predicción local: post-POC (D11).
5. **Red**: escalera de D10 — SDK nativo de Colyseus, validado con spike temprano; fallbacks documentados que conservan el servidor.
6. **HUD/debug — Dear ImGui** (backend oficial SDL GPU): ping, tick, velocidad, overlay de debug (posición autoritativa, colisión del nivel, latencia artificial configurable). El HUD "de juego" definitivo es post-POC; ImGui es la herramienta de desarrollo.

Bucle del cliente: render a la tasa del monitor, totalmente desacoplado del tick de red (20 Hz); el muestreo de input se envía a tasa fija con número de secuencia.

## Código compartido y constantes

- `shared/` (TS) sigue siendo la fuente de verdad de **constantes** (tick rate, límites de input, parámetros de física) y **tipos de mensajes**, usada por el servidor.
- El cliente C++ no puede importarla: un paso de generación (`npm run gen:constants`) emite un header `client-native/src/generated/constants.h` desde `shared/`, eliminando la divergencia silenciosa. Mientras el cliente solo interpola (POC), las constantes que necesita son pocas (tick, interpolación); el generador evita el drift cuando lleguen la predicción y el port de `stepCar` (D11).

## Protocolo

| Dirección | Mensaje | Contenido | Vía |
|---|---|---|---|
| C → S | `input` | `{ seq, throttle: -1..1, steer: -1..1, brake: bool }` | mensaje de sala |
| S → C | estado del mundo | `PlayerState { x, y, heading, vx, vy, lastProcessedSeq }` | Schema (o snapshot msgpack/JSON según spike) |
| S → C | `event` | checkpoints, recompensas, avisos | mensaje de sala |
| C ⇄ S | `ping`/`pong` | `{ sentAt }` | mensaje de sala |

## Pipeline de niveles (Blender → glTF)

- Blender es el editor de niveles. Un `.blend` por mapa en `assets/levels/src/`, exportado a glTF en `assets/levels/`.
- **Convención de nombres** sobre nodos/objetos: `col_*` (geometría de colisión: su huella en planta se convierte en polígonos 2D para el servidor), `zone_*` (zonas de interacción), `spawn_*` (puntos de aparición). El resto es decorado: el cliente lo renderiza, el servidor lo ignora.
- El cliente carga el mismo glTF con cgltf y lo renderiza con nuestros pipelines.

## Estructura del repositorio (objetivo)

```
MMO4wheels/
├── package.json          # npm workspaces: shared, server, client (sonda web)
├── client-native/        # C++ + SDL3 + CMake — el cliente del juego
│   ├── CMakeLists.txt    # FetchContent: SDL3, GLM, cgltf, stb, imgui; glslc para shaders
│   ├── shaders/          # GLSL 450 (fuente) → .spv generados en build
│   └── src/
│       ├── platform/     # arranque, ventana, bucle, input
│       ├── render/       # device, pipelines, passes, post, sombras
│       ├── scene/        # carga glTF, materiales, transforms
│       ├── world/        # réplica de estado, interpolación
│       ├── net/          # conexión (SDK nativo / WS)
│       └── generated/    # constants.h (no editar a mano)
├── server/               # Node + Colyseus (sin cambios de fondo)
├── shared/               # constantes, tipos de mensajes, stepCar (TS)
├── client/               # sonda web M0 (Phaser) — se retira al cerrar el POC nativo
├── assets/
│   ├── levels/           # glTF exportados (+ src/ con .blend)
│   └── models/           # coche y props (CC0: Kenney 3D, Quaternius)
└── docs/
```

## El camino a los dos futuros

- **MMO real**: la sala ya acepta N conexiones; los demás coches se renderizan con la misma interpolación. Pendiente de siempre en MMOs (interés/área, login real), nada lo bloquea.
- **Single player puro**: el binario nativo arranca el servidor Node local como proceso sidecar y se conecta a `localhost` (latencia ~0). Si el sidecar resulta incómodo para distribuir, el port del servidor es una decisión post-POC — la lógica (sala + `stepCar` + niveles glTF) está deliberadamente poco acoplada a Colyseus.
