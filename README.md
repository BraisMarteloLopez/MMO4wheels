# MMO4wheels

Juego de coches 2D en pixel art con vista cenital, planteado como **single MMO**: se juega como experiencia individual, pero sobre una arquitectura cliente‑servidor con servidor autoritativo desde el día 1. Esto deja abiertas las dos puertas de futuro: convertirlo en multijugador real o empaquetarlo como single player puro (servidor local).

**Estado actual: fase de diseño.** El objetivo inmediato es un POC (Proof of Concept) en el que se pueda conducir un coche por un escenario y realizar interacciones reales con el servidor (movimiento autoritativo, zonas de interacción, persistencia del mundo).

## Decisiones mayores

| Decisión | Elección |
|---|---|
| Estética | Pixel art 2D, vista cenital |
| Arquitectura | Cliente‑servidor, servidor autoritativo (también en single player) |
| Cliente | Phaser 3 + TypeScript + Vite (navegador) |
| Servidor | Node.js + Colyseus + TypeScript |
| Código compartido | Paquete `shared/` (física del coche, tipos, constantes) |
| Física | Modelo arcade propio, determinista, ejecutable en cliente y servidor |

La justificación de cada decisión está en [docs/01-vision-y-decisiones.md](docs/01-vision-y-decisiones.md).

## Documentación

1. [Visión y decisiones](docs/01-vision-y-decisiones.md) — qué es el juego y por qué cada decisión mayor.
2. [Arquitectura](docs/02-arquitectura.md) — cliente, servidor, protocolo, sincronización y estructura del repo.
3. [Plan del POC](docs/03-plan-poc.md) — alcance, hitos M0–M4, criterios de éxito y riesgos.

## Estructura prevista del repositorio

```
MMO4wheels/
├── client/   # Phaser 3 + Vite (navegador)
├── server/   # Node.js + Colyseus
├── shared/   # física, tipos de mensajes, constantes
└── docs/     # diseño y planificación
```
