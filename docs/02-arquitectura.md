# Arquitectura

## Vista general

```
┌──────────── Cliente nativo (C++ + raylib) ────────────┐      ┌────────────── Servidor (Node.js) ─────────────┐
│                                                        │      │  Colyseus + TypeScript                         │
│  Plataforma (raylib): ventana, input, glTF, audio      │  WS  │                                                │
│  Renderer propio (OpenGL 3.3 core, GLSL 330):          │◄────►│  · WorldRoom (sala autoritativa)               │
│   · cámara aérea, materiales, luz direccional          │      │  · Simulación 2D en planta a tick fijo (20 Hz) │
│   · niebla, tonemap + bloom, sombras simples           │      │  · stepCar() + colisiones + zonas              │
│  Mundo: snapshots interpolados (~100 ms)               │      │  · Validación de inputs                        │
│  Red: SDK nativo Colyseus (spike) / WS propio          │      │  · Estado: Colyseus Schema                     │
│  HUD/debug: ping, tick, overlay                        │      │  · Persistencia (POC: JSON → interfaz Storage) │
└────────────────────────────────────────────────────────┘      └────────────────────────────────────────────────┘
                         ▲                                                        ▲
                         │ mallas y materiales                                    │ colisión 2D y zonas (nodos col_*/zone_*)
                  ┌──────┴──────────────────────────────────────────────────────┐│
                  │  assets/levels/*.gltf — exportado desde Blender             ├┘
                  │  (única fuente de verdad del nivel)                         │
                  └─────────────────────────────────────────────────────────────┘
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

1. **Plataforma — raylib**: ventana, contexto OpenGL (3.3 core en escritorio; *no* OpenGL ES, que es el backend web/embebido de raylib), input, carga de modelos glTF, audio. Es la frontera de "fontanería que no escribimos".
2. **Renderer propio**: todo lo que se ve es código nuestro — cámara aérea que sigue al coche, pase principal con luz direccional y niebla por distancia, sombras simples (shadow map básico, con *blob shadows* como salida digna si se enquista), post-procesado mínimo (tonemapping + bloom ligero) vía render textures y shaders GLSL 330 propios. Subir al backend 4.3 de raylib (compute/SSBO) es recompilar, cuando haga falta — no en el POC.
3. **Mundo**: réplica local del estado del servidor. Entidades renderizadas con **interpolación** entre los dos últimos snapshots (~100 ms de retraso de presentación). Predicción local: post-POC (D11).
4. **Red**: escalera de D10 — SDK nativo de Colyseus, validado con spike temprano; fallbacks documentados que conservan el servidor.
5. **HUD/debug**: ping, tick del servidor, velocidad; overlay de debug (posición autoritativa, colisión del nivel en wireframe, latencia artificial configurable).

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
- El cliente carga el mismo glTF con raylib y lo renderiza con nuestros shaders.

## Estructura del repositorio (objetivo)

```
MMO4wheels/
├── package.json          # npm workspaces: shared, server, client (sonda web)
├── client-native/        # C++ + raylib + CMake — el cliente del juego
│   ├── CMakeLists.txt
│   └── src/
│       ├── platform/     # arranque, ventana, bucle
│       ├── render/       # cámara, pases, shaders, post
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
