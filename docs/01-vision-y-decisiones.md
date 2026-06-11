# Visión y decisiones mayores

## Visión

MMO4wheels es un juego de coches con vista cenital y estética pixel art 2D. Se plantea como **single MMO**: un mundo online persistente que se vive como experiencia individual. El jugador conduce por un mundo que existe en el servidor, con estado que persiste entre sesiones.

Que sea "single" hoy no cambia la arquitectura: el servidor es la autoridad del juego desde el primer día. Así, las dos evoluciones posibles quedan baratas:

- **Hacia MMO real**: admitir más conexiones en el mismo mundo (la infraestructura de salas y sincronización ya existe).
- **Hacia single player puro**: empaquetar el servidor como proceso local junto al cliente (la arquitectura no cambia, solo el despliegue).

### Pilares del juego (para guiar decisiones)

1. **El feel de conducción manda.** Si conducir no es satisfactorio, nada más importa. Físicas arcade: accesibles, con margen para el dominio (derrapes, trazadas).
2. **El mundo es persistente.** Lo que haces queda: tu posición, tu progreso. Es la esencia de la parte "MMO".
3. **El servidor es la verdad.** El cliente pinta y predice; nunca decide.

## Registro de decisiones

Formato ligero tipo ADR: decisión, justificación y consecuencias. Si una decisión se revierte, se anota aquí en lugar de borrarla.

### D1 — Pixel art 2D con vista cenital

**Decisión de partida del proyecto.** Consecuencias técnicas: render 2D con sprites y tilemaps; el coche es un sprite que rota; resolución de trabajo baja (tiles de 16×16 o 32×32 px) con escalado entero para mantener el pixel art nítido.

### D2 — Cliente‑servidor con servidor autoritativo, incluso en single player

**Justificación**: es el requisito que define el proyecto. Un servidor autoritativo evita reescrituras futuras si el juego se abre a multijugador, y hace trivial la persistencia del mundo.

**Consecuencias**:
- Toda la lógica de juego relevante (movimiento, colisiones, interacciones, economía) vive en el servidor.
- El cliente es render + input + técnicas de ocultación de latencia (interpolación, predicción).
- Coste asumido: más complejidad inicial que un juego local. El POC existe precisamente para validar que este coste es manejable.

### D3 — Stack TypeScript: Phaser 3 (cliente) + Node.js/Colyseus (servidor)

**Elegido frente a**: Godot en ambos lados, Godot + servidor custom, Unity + servidor custom.

**Justificación**:
- **Un solo lenguaje (TypeScript) en cliente, servidor y código compartido.** Esto habilita la pieza clave del diseño: ejecutar exactamente la misma física en ambos lados (ver D4).
- **Colyseus** está diseñado justo para nuestro caso: salas con estado autoritativo en servidor, sincronización automática de estado (Schema con patches binarios), ciclo de simulación a tick fijo.
- **Navegador como plataforma**: distribución por URL (natural para un MMO), iteración rápida con Vite, sin builds nativos durante el POC.

**Consecuencias**: el cliente vive en el navegador (limitaciones de WebSocket frente a UDP: aceptable para este género y esencial para web). Versiones concretas a fijar en el hito M0 con lo último estable (Node LTS, TypeScript 5.x, Colyseus 0.16.x; evaluar Phaser 4 si ya es estable en ese momento — no cambia el diseño).

### D4 — Física de coche arcade propia y compartida (sin motor de física)

**Justificación**: para conducción top‑down arcade, un modelo propio de ~100 líneas (aceleración, fricción, giro dependiente de velocidad, control de derrape) da mejor feel y más control que un motor de física realista. Además es una **función pura y determinista** (`stepCar(estado, input, dt) → estado`), lo que permite:
- Ejecutarla en el servidor como simulación autoritativa.
- Ejecutarla en el cliente para predicción local, con resultados (casi) idénticos.

**Consecuencias**: no usamos Matter.js/Box2D para el coche. Las colisiones con el escenario se resuelven a mano contra el tilemap (suficiente para el POC). Si en el futuro hacen falta colisiones complejas coche‑coche, se reevalúa.

### D5 — Monorepo con npm workspaces: `client/`, `server/`, `shared/`

**Justificación**: el paquete `shared/` (física, tipos de mensajes, constantes de juego) debe importarse desde cliente y servidor sin fricción ni publicación. Un monorepo con workspaces lo resuelve con cero infraestructura.

### D6 — Assets placeholder CC0 durante el POC

**Justificación**: el arte propio no es objetivo del POC. Se usarán packs CC0 (p. ej. Kenney *Racing Pack* / packs pixel) para coche y tiles. El pipeline de arte propio se decide después del POC.

### D7 — Mapas con Tiled (formato JSON)

**Justificación**: Tiled es el editor estándar de tilemaps; Phaser carga su JSON de forma nativa, y **el servidor lee el mismo archivo** para construir su geometría de colisión. Una sola fuente de verdad para el escenario.

## Preguntas abiertas (post‑POC, no bloquean)

- ¿Qué *es* el juego más allá de conducir? (misiones, carreras, exploración, economía…) El POC no lo necesita; el feel y la arquitectura son previos.
- Resolución y tamaño de tile definitivos (16 vs 32 px) — se decidirá probando con los assets placeholder en M1.
- Pipeline de arte propio y herramienta (Aseprite, etc.).
- Despliegue real del servidor (VPS, Render, Fly.io…) — para el POC basta local, con un deploy opcional en M4.
