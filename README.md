# MMO4wheels

Juego de coches con vista aérea y estética **low-poly 3D** (referencia de tono: los mapas de Risk of Rain 2), planteado como **single MMO**: se juega como experiencia individual, pero sobre una arquitectura cliente‑servidor con servidor autoritativo desde el día 1. Esto deja abiertas las dos puertas de futuro: convertirlo en multijugador real o empaquetarlo como single player puro (servidor local).

El cliente es un binario nativo de escritorio con **renderer propio** (C++ + raylib sobre OpenGL): los shaders, la luz, la niebla y el post-procesado son código nuestro. La simulación del servidor es plana (x, y, rumbo); el 3D es presentación.

El objetivo actual es un POC (Proof of Concept): conducir un coche por un escenario con atmósfera propia y realizar interacciones reales con el servidor (movimiento autoritativo, zonas de interacción, persistencia del mundo).

**Estado: re-planificado tras pivote a 3D nativo (2026‑06).** El esqueleto web M0 está completado y su servidor se conserva tal cual; el cliente web queda como sonda de debug hasta que el cliente nativo lo sustituya. Siguiente hito: **M0′ — toolchain C++/raylib** (ver [plan del POC](docs/03-plan-poc.md)).

## Decisiones mayores

| Decisión | Elección |
|---|---|
| Estética | Low-poly 3D con foco artístico, vista aérea |
| Arquitectura | Cliente‑servidor, servidor autoritativo (también en single player) |
| Cliente | C++ + raylib, renderer propio (OpenGL 3.3 core, GLSL 330) |
| Servidor | Node.js + Colyseus 0.17 + TypeScript |
| Simulación | Plana (2D en planta) a tick fijo, presentada en 3D |
| Niveles | Blender → glTF, única fuente para render (cliente) y colisión/zonas (servidor) |
| Assets POC | CC0 low-poly (Kenney 3D, Quaternius) |

La justificación de cada decisión (y la historia de las sustituidas) está en [docs/01-vision-y-decisiones.md](docs/01-vision-y-decisiones.md).

## Cómo ejecutar (estado actual: esqueleto M0 web)

Hasta que el cliente nativo aterrice (M0′), lo ejecutable es el esqueleto verificado del M0: servidor de juego + sonda web.

Requisitos: Node.js ≥ 22.

```bash
npm install
npm run dev        # levanta servidor (ws://localhost:2567) y sonda web (http://localhost:5173)
npm run smoke      # con el dev levantado: test de conectividad join + estado + ping/pong
npm run typecheck  # comprueba tipos en los paquetes TS
```

Las instrucciones de compilación del cliente nativo se añadirán con M0′.

## Documentación

1. [Visión y decisiones](docs/01-vision-y-decisiones.md) — qué es el juego y por qué cada decisión mayor (con historial).
2. [Arquitectura](docs/02-arquitectura.md) — cliente nativo, servidor, protocolo, pipeline de niveles.
3. [Plan del POC](docs/03-plan-poc.md) — alcance, hitos M0′–M5′, riesgos y criterios de éxito.

## Estructura del repositorio (objetivo)

```
MMO4wheels/
├── client-native/   # C++ + raylib + CMake: el cliente del juego (desde M0′)
├── server/          # Node.js + Colyseus: sala del mundo, simulación a tick fijo
├── shared/          # constantes, tipos de mensajes y física (TS); genera constants.h
├── client/          # sonda web M0 (Phaser) — se retira al cerrar el POC nativo
├── assets/          # niveles (Blender → glTF) y modelos CC0
└── docs/            # diseño y planificación
```
