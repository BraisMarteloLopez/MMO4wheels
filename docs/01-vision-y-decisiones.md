# Visión y decisiones mayores

## Visión

MMO4wheels es un juego de coches con vista aérea y estética **low-poly 3D con foco artístico** (referencia de tono: los mapas de Risk of Rain 2 — geometría simple, atmósfera fuerte). Se plantea como **single MMO**: un mundo online persistente que se vive como experiencia individual. El jugador conduce por un mundo que existe en el servidor, con estado que persiste entre sesiones.

Que sea "single" hoy no cambia la arquitectura: el servidor es la autoridad del juego desde el primer día. Así, las dos evoluciones posibles quedan baratas:

- **Hacia MMO real**: admitir más conexiones en el mismo mundo (la infraestructura de salas y sincronización ya existe).
- **Hacia single player puro**: empaquetar el servidor como proceso local junto al cliente (la arquitectura no cambia, solo el despliegue).

> **Nota (2026‑06)**: el proyecto nació como pixel art 2D en navegador; en el cierre de M0 se pivotó a 3D low-poly nativo con renderer propio. Las decisiones D1, D3 (parcial), D4, D6 y D7 quedan sustituidas por D8–D12. Se conservan anotadas como historia.

### Pilares del juego (para guiar decisiones)

1. **El feel de conducción manda.** Si conducir no es satisfactorio, nada más importa. Físicas arcade: accesibles, con margen para el dominio (derrapes, trazadas).
2. **El mundo es persistente.** Lo que haces queda: tu posición, tu progreso. Es la esencia de la parte "MMO".
3. **El servidor es la verdad.** El cliente pinta y predice; nunca decide.
4. **La atmósfera manda en lo visual.** Luz, niebla, color y composición por encima del detalle: low-poly como elección estética, no como limitación.

## Registro de decisiones

Formato ligero tipo ADR: decisión, justificación y consecuencias. Si una decisión se revierte, se anota aquí en lugar de borrarla.

### D1 — Pixel art 2D con vista cenital ⚠️ *sustituida por D8*

Decisión de partida del proyecto. Sustituida en 2026‑06: la vista aérea se mantiene; la estética pasa a low-poly 3D.

### D2 — Cliente‑servidor con servidor autoritativo, incluso en single player ✅ *vigente*

**Justificación**: es el requisito que define el proyecto. Un servidor autoritativo evita reescrituras futuras si el juego se abre a multijugador, y hace trivial la persistencia del mundo.

**Consecuencias**:
- Toda la lógica de juego relevante (movimiento, colisiones, interacciones, economía) vive en el servidor.
- El cliente es render + input + técnicas de ocultación de latencia (interpolación, predicción).
- Coste asumido: más complejidad inicial que un juego local. El POC existe precisamente para validar que este coste es manejable.

### D3 — Stack TypeScript: Phaser (cliente) + Node.js/Colyseus (servidor) ⚠️ *cliente sustituido por D8; servidor vigente vía D10*

Elegida frente a Godot/Unity por el monolenguaje TS y la distribución web. El esqueleto M0 con esta pila se construyó y verificó (Phaser 4, Colyseus 0.17, smoke test de conectividad). Al pivotar a cliente nativo (D8), la parte de cliente queda sustituida; el servidor Node/Colyseus se conserva (D10). El cliente web M0 se mantiene temporalmente como sonda de debug del servidor y se retirará al cerrar el POC nativo.

### D4 — Física de coche arcade propia y compartida (sin motor de física) ⚠️ *transformada por D9/D11*

El modelo arcade propio y determinista (`stepCar(estado, input, dt)`) **sigue vigente como diseño**. Lo que muere con el cliente TS es el truco de *compartir el mismo código* entre cliente y servidor. La nueva estrategia de ejecución está en D11.

### D5 — Monorepo: `client/`, `server/`, `shared/` ✅ *vigente, ampliada*

Los workspaces npm siguen para el lado TS (servidor, shared, sonda web). El cliente nativo entra como proyecto CMake en `client-native/` dentro del mismo repo. Estructura objetivo en `docs/02-arquitectura.md`.

### D6 — Assets placeholder CC0 (pixel art) ⚠️ *sustituida por D12*

### D7 — Mapas con Tiled (formato JSON) ⚠️ *sustituida por D12*

### D8 — Cliente nativo 3D low-poly con renderer propio: C++ + SDL3 GPU (2026‑06)

**Decisión**: el cliente deja el navegador y pasa a ser un binario nativo para **Windows 10/11 x64 (plataforma objetivo)**, escrito en **C++**, con **SDL3** como capa de plataforma (ventana, input, audio) y **nuestro propio render** construido sobre la **SDL3 GPU API**. Linux se mantiene como entorno de build y verificación headless (agente/CI); macOS, posible más adelante — el API gráfico moderno de SDL (command buffers, render passes y pipelines explícitos; backends Vulkan/D3D12/Metal). Los shaders son nuestros, en GLSL `#version 450`, compilados offline a **SPIR-V** con `glslc` como paso de build; durante el POC se fuerza el backend Vulkan en Windows y Linux para manejar un único formato de shader (el cross-compile a DX12/Metal con SDL_shadercross queda para cuando importe). Kit alrededor: **GLM** (matemáticas), **cgltf** (carga de glTF), **stb_image**, **Dear ImGui** (HUD de debug; tiene backend oficial de SDL GPU).

> **Revisión (2026‑06, antes de implementar)**: la formulación inicial de D8 era raylib sobre OpenGL 3.3 core. Se revisó al pesar que toda la familia OpenGL está congelada (la última versión, 4.6, es de 2017): si el motivo del renderer propio es aprender gráficos con proyección de futuro, la inversión debe ir a un API vivo. SDL3 GPU se eligió frente a Vulkan directo (cuyo peaje de fontanería dobla otra vez el calendario) y frente a WebGPU/Dawn (dependencia pesada en C++): da los conceptos GPU modernos con la sincronización gestionada, y la misma dependencia resuelve la capa de plataforma que aportaba raylib.

**Justificación**: interés explícito en gráficos a bajo nivel sobre un API con futuro y control artístico total, con los costes sobre la mesa: sin las pilas de raylib (carga de modelos, cámara, texto) el POC sube a ~17–27 sesiones (ver plan), y SDL3 GPU es un API joven (estable desde 2025) con menos literatura que OpenGL — se asume, apoyándonos en los ejemplos oficiales y el backend de ImGui.

**Consecuencias**:
- Distribución por descarga de binario (la distribución por URL murió con el navegador).
- Paso de compilación de shaders en el build (GLSL → SPIR-V).
- macOS deja de estar vetado (backend Metal vía shadercross), aunque sigue fuera del POC.
- Pipeline de arte 3D (D12) y re-plan completo del POC (`docs/03-plan-poc.md`).
- El riesgo principal del proyecto sigue siendo el *scope creep de motor*; misma mitigación: lista cerrada de features de render para el POC.

### D9 — Simulación plana presentada en 3D (2026‑06)

**Decisión**: la simulación autoritativa del servidor sigue siendo **2D en planta** (x, y, rumbo) — exactamente el diseño que ya teníamos —; el cliente la presenta con modelos y escenarios low-poly 3D. La altura del escenario es decorativa.

**Justificación**: separa el pivote visual del coste de la física 3D (ruedas raycast, suspensión, colisión contra mallas en servidor). Mantiene casi intacto el diseño de servidor, protocolo y persistencia.

**Consecuencias**: los mapas se componen visualmente en 3D pero su geometría jugable es un plano con obstáculos 2D. La evolución a desniveles reales (heightmap con proyección del coche a la superficie como primer paso) queda explícitamente abierta para después del POC.

### D10 — El servidor Node/Colyseus se mantiene (2026‑06)

**Decisión**: se conserva el servidor verificado en M0 (sala autoritativa a 20 Hz, persistencia, matchmaking). El cliente nativo se conecta mediante esta escalera, a validar con un spike temprano:

1. **SDK nativo oficial de Colyseus** (librería C estática, `colyseus/native-sdk`; existe ejemplo con raylib). Compatibilidad con 0.17 por confirmar.
2. *Fallback A*: sala Colyseus usada solo con mensajes (msgpack/JSON), sin depender del schema binario en el cliente.
3. *Fallback B*: endpoint WebSocket plano en nuestro propio proceso Node (la lógica de sala/tick/física se conserva), cliente con IXWebSocket y snapshots JSON/msgpack.

**Justificación**: el servidor está hecho, probado, y su sustitución no aporta nada al POC. Si el spike revela fricción insalvable, los fallbacks degradan con elegancia sin tirar la lógica.

### D11 — Estrategia de física sin código compartido (2026‑06)

**Decisión**: para el POC, el servidor es el único que ejecuta `stepCar()` (TypeScript) y el cliente **interpola** snapshots (~100 ms de retraso de presentación). La predicción local llega después del POC, eligiendo entre: (a) portar `stepCar` a C++ (~100 líneas; las **constantes se generan** desde `shared/` a un header C++ para que no diverjan), o (b) escribir la física en C y compilarla a nativo (cliente) y WASM (servidor Node) — un solo código otra vez.

**Justificación**: en un single MMO la latencia típica es baja y la interpolación pura puede bastar para el feel; no pagamos la duplicación hasta demostrar que hace falta. La decisión (a) vs (b) se toma con datos del POC.

### D12 — Pipeline de arte: Blender → glTF; assets CC0 low-poly (2026‑06)

**Decisión**: los escenarios se componen en **Blender** y se exportan a **glTF**, que es la única fuente de verdad del nivel: el cliente renderiza sus mallas y materiales; el servidor (Node) lee el mismo archivo (glTF es JSON + binario) y extrae geometría de colisión 2D y zonas de interacción a partir de una **convención de nombres** en los nodos (`col_*`, `zone_*`, `spawn_*`). Para el POC, modelos placeholder CC0 low-poly (Kenney 3D, Quaternius).

**Justificación**: Blender como editor de niveles es el patrón estándar cuando no hay editor propio; la convención de nombres evita herramientas intermedias. Sustituye a Tiled (D7), conservando su virtud: una sola fuente para ver y para colisionar.

## Preguntas abiertas (post‑POC, no bloquean)

- ¿Qué *es* el juego más allá de conducir? (misiones, carreras, exploración, economía…)
- Dirección de arte concreta: paleta, iluminación tipo, identidad visual propia más allá de la referencia RoR2.
- Predicción local: ¿port C++ o física en C+WASM? (D11, decidir con datos del POC).
- Desniveles reales post-POC: heightmap + proyección vs física 3D completa.
- ¿Port del servidor a otro runtime si el sidecar Node molesta en el empaquetado single player?
- Soporte macOS post-POC (la SDL3 GPU API trae backend Metal; faltaría el cross-compile de shaders con SDL_shadercross).
