# MMO4wheels

Juego de coches 2D en pixel art con vista cenital, planteado como **single MMO**: se juega como experiencia individual, pero sobre una arquitectura cliente‑servidor con servidor autoritativo desde el día 1. Esto deja abiertas las dos puertas de futuro: convertirlo en multijugador real o empaquetarlo como single player puro (servidor local).

El objetivo actual es un POC (Proof of Concept) en el que se pueda conducir un coche por un escenario y realizar interacciones reales con el servidor (movimiento autoritativo, zonas de interacción, persistencia del mundo).

**Estado: M0 completado** — monorepo funcionando, cliente Phaser conectándose al servidor Colyseus con estado sincronizado y medición de ping. Siguiente hito: M1, conducir en local (ver [plan del POC](docs/03-plan-poc.md)).

## Cómo ejecutar

Requisitos: Node.js ≥ 22.

```bash
npm install
npm run dev        # levanta servidor (ws://localhost:2567) y cliente (http://localhost:5173)
```

Abre <http://localhost:5173>: verás el escenario placeholder con el coche y un HUD con el estado de conexión, el ping y el tick del servidor.

Otros comandos útiles:

```bash
npm run smoke      # con el dev levantado: test de conectividad join + estado + ping/pong
npm run typecheck  # comprueba tipos en los tres paquetes
npm run build      # build de producción del cliente
```

## Decisiones mayores

| Decisión | Elección |
|---|---|
| Estética | Pixel art 2D, vista cenital |
| Arquitectura | Cliente‑servidor, servidor autoritativo (también en single player) |
| Cliente | Phaser 4 + TypeScript + Vite (navegador) |
| Servidor | Node.js + Colyseus 0.17 + TypeScript |
| Código compartido | Paquete `shared/` (física del coche, tipos, constantes) |
| Física | Modelo arcade propio, determinista, ejecutable en cliente y servidor |

La justificación de cada decisión está en [docs/01-vision-y-decisiones.md](docs/01-vision-y-decisiones.md).

## Documentación

1. [Visión y decisiones](docs/01-vision-y-decisiones.md) — qué es el juego y por qué cada decisión mayor.
2. [Arquitectura](docs/02-arquitectura.md) — cliente, servidor, protocolo, sincronización y estructura del repo.
3. [Plan del POC](docs/03-plan-poc.md) — alcance, hitos M0–M4, criterios de éxito y riesgos.

## Estructura del repositorio

```
MMO4wheels/
├── client/   # Phaser 4 + Vite (navegador): escenas, capa de red
├── server/   # Node.js + Colyseus: sala del mundo, simulación a tick fijo
├── shared/   # constantes y tipos de mensajes (la física llega en M1)
└── docs/     # diseño y planificación
```
