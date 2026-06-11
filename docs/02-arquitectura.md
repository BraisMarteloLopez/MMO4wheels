# Arquitectura

## Vista general

```
┌──────────────── Navegador ────────────────┐        ┌────────────── Servidor (Node.js) ─────────────┐
│  Cliente — Phaser 3 + TypeScript           │        │  Colyseus + TypeScript                         │
│                                            │        │                                                │
│  · Escenas: Boot, Game, HUD                │  WebSocket  · WorldRoom (sala autoritativa)            │
│  · Input → comandos de input               │◄──────►│  · Bucle de simulación a tick fijo (20 Hz)     │
│  · Render del estado + interpolación       │        │  · Validación de inputs                        │
│  · Predicción local + reconciliación       │        │  · Estado del mundo (Colyseus Schema)          │
│  · Overlay de debug (ping, pos. servidor)  │        │  · Zonas de interacción, eventos               │
└────────────────────────────────────────────┘        │  · Persistencia (POC: JSON → interfaz Storage) │
                      ▲                               └────────────────────────────────────────────────┘
                      │ importan ambos                                  ▲
            ┌─────────┴─────────────┐                                   │ lee el mismo mapa
            │  shared/              │                          ┌────────┴────────┐
            │  · stepCar() (física) │                          │ mapa Tiled JSON │
            │  · tipos de mensajes  │                          │ (capa colisión) │
            │  · constantes (tick…) │                          └─────────────────┘
            └───────────────────────┘
```

Principio rector: **el servidor simula, el cliente presenta**. Todo lo que afecta al estado del juego se decide en el servidor; el cliente aplica técnicas estándar para que la latencia no se note.

## Servidor

- **Colyseus `WorldRoom`**: una sala = el mundo (en el POC, una única sala). Acepta conexiones, asocia cada cliente a su coche y ejecuta la simulación.
- **Tick fijo a 20 Hz** (`setSimulationInterval`): en cada tick consume los inputs recibidos de cada cliente, llama a `stepCar()` de `shared/`, resuelve colisiones contra el tilemap y evalúa zonas de interacción. `dt` fijo = simulación estable y reproducible.
- **Estado como Colyseus Schema**: Colyseus difunde automáticamente solo los cambios (patches binarios) a los clientes. No escribimos código de serialización.
- **Validación**: el cliente solo envía *intenciones* (ejes de input), nunca posiciones. Inputs fuera de rango se recortan. Es lo que hace al servidor autoritativo de verdad.
- **Persistencia**: detrás de una interfaz `Storage` (clave‑valor por jugador). Implementación del POC: archivo JSON en disco. Sustituible por SQLite/Postgres sin tocar la lógica de la sala.
- **Identidad** (POC): id anónimo generado por el cliente y guardado en `localStorage`; el servidor lo usa como clave de persistencia. Autenticación real queda fuera del POC.

## Cliente

- **Phaser 3 + Vite**. Escenas: `Boot` (carga de assets/mapa), `Game` (mundo), `HUD` (velocidad, ping, monedas, avisos del servidor).
- **Render del mapa**: tilemap de Tiled. Cámara que sigue al coche. Zoom con escalado entero y `pixelArt: true` para nitidez.
- **Input**: muestreo de teclado por frame (acelerar, frenar/marcha atrás, girar) → mensaje `input` al servidor con número de secuencia.
- **Entidades remotas** (y el propio coche en modo simple): **interpolación** — se renderiza el estado del servidor con ~100 ms de retraso, interpolando entre los dos últimos snapshots. Movimiento suave aunque los ticks lleguen a 20 Hz.
- **Coche propio** (modo avanzado): **predicción + reconciliación** — el cliente aplica `stepCar()` localmente al instante (respuesta inmediata) y, al recibir el estado confirmado del servidor (`lastProcessedSeq`), re‑aplica los inputs aún no confirmados. Si hay divergencia, corrección suave hacia la posición autoritativa.
- **Herramientas de validación**: latencia artificial configurable (para probar el feel con 100–200 ms) y overlay de debug que pinta la posición autoritativa junto a la predicha.

## Código compartido (`shared/`)

La pieza que justifica el stack monolenguaje:

- `stepCar(state, input, dt): state` — física arcade pura y determinista. Modelo: rumbo `θ`; la velocidad de giro escala con la velocidad lineal (parado no giras); aceleración a lo largo del rumbo; fricción y resistencia; velocidad máxima; amortiguación de la velocidad lateral parametrizada (= control del derrape).
- Tipos de mensajes y constantes (tick rate, dimensiones de tile, límites de input).
- Sin dependencias de Phaser ni de Colyseus: importable desde ambos lados.

## Protocolo

| Dirección | Mensaje | Contenido | Vía |
|---|---|---|---|
| C → S | `input` | `{ seq, throttle: -1..1, steer: -1..1, brake: bool }` | mensaje Colyseus |
| S → C | estado del mundo | `PlayerState { x, y, heading, vx, vy, lastProcessedSeq }`, mundo | Colyseus Schema (auto) |
| S → C | `event` | checkpoints, recompensas, avisos (“+10 monedas”) | mensaje Colyseus |

Transporte: WebSocket (gestionado por Colyseus). Serialización del estado: la binaria de Schema. Mensajes puntuales: JSON. Suficiente para el POC; optimizar no es objetivo todavía.

## Mapa y colisiones

- **Una sola fuente de verdad**: el JSON de Tiled. Capas visuales (suelo, decoración) + capa de colisión (tiles sólidos) + capa de objetos (spawn, zonas de interacción).
- El **cliente** lo usa para pintar; el **servidor** lo carga al arrancar y construye una rejilla de colisión. Colisión coche‑escenario: círculo del coche contra tiles sólidos (AABB), resuelta en el servidor (y en la predicción del cliente con el mismo código de `shared/`).

## Estructura del repositorio

```
MMO4wheels/
├── package.json            # npm workspaces: client, server, shared
├── client/
│   ├── src/
│   │   ├── scenes/         # Boot, Game, HUD
│   │   ├── net/            # conexión Colyseus, interpolación, predicción
│   │   └── main.ts
│   └── public/assets/      # sprites, tiles, mapa Tiled
├── server/
│   └── src/
│       ├── rooms/WorldRoom.ts
│       ├── world/          # carga de mapa, colisiones, zonas
│       └── storage/        # interfaz Storage + impl. JSON
├── shared/
│   └── src/
│       ├── physics.ts      # stepCar()
│       ├── messages.ts
│       └── constants.ts
└── docs/
```

## El camino a los dos futuros

- **MMO real**: la `WorldRoom` ya acepta N conexiones; los demás jugadores se renderizan con la interpolación que ya existe para el propio coche. Lo que faltaría es lo de siempre en MMOs (interés/área de visión, sharding, login real), pero ninguna decisión del POC lo bloquea.
- **Single player puro**: se distribuye el servidor Node como proceso local (p. ej. con Electron/Tauri o un lanzador) y el cliente se conecta a `localhost`. Latencia ~0 ⇒ la predicción se vuelve invisible pero inofensiva.
