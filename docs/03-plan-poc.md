# Plan del POC

> Re-planificado en 2026‑06 tras el pivote a cliente nativo 3D (D8 — revisada el mismo mes: renderer propio sobre **SDL3 GPU API**, no raylib/OpenGL). El plan original 2D/web queda en el historial de git. El hito M0 web se completó y su servidor **se conserva**; el cliente web pasa a ser sonda de debug.

## Objetivo

Demostrar, con el mínimo juego posible, que la base de MMO4wheels funciona:

1. **Conducir es agradable** — coche top-down con feel arcade.
2. **El look low-poly artístico es alcanzable con nuestro render** — una escena con luz, niebla y composición que "ya parece un juego".
3. **El servidor es la autoridad** — el cliente no decide su posición; con latencia sigue siendo jugable.
4. **El mundo persiste** — cierras y vuelves: estás donde estabas, con tu progreso.

## Alcance

**Dentro**: cliente nativo C++ con renderer propio sobre SDL3 GPU (kit: GLM, cgltf, stb_image, Dear ImGui para debug; shaders GLSL 450 → SPIR-V con `glslc`, backend Vulkan en Win/Linux); un coche; un escenario compuesto en Blender; conexión al servidor Colyseus existente; simulación autoritativa plana a 20 Hz; interpolación; una zona de interacción server-side; persistencia simple; HUD de debug con ImGui.

**Features de render del POC (lista CERRADA — el antídoto del scope creep de motor)**: cámara aérea, luz direccional, niebla por distancia, tonemapping + bloom ligero, sombras simples (shadow map básico **o** blob shadows si el shadow map se enquista), carga y render de glTF con materiales de color plano.

**Fuera** (explícitamente): PBR, GI, SSAO, antialiasing temporal, partículas GPU, animación esquelética, agua, día/noche, editor in-game, cross-compile de shaders (DX12/Metal/macOS), otros jugadores visibles, sonido, menús, login real, arte propio definitivo, móvil, optimización de red, despliegue en producción. Cada tentación de render va a un backlog, no al POC.

## Modo de ejecución (actualizado 2026‑06)

Reparto de roles acordado: **el usuario toma las decisiones de alto nivel y de diseño y hace las pruebas visuales/de feel; Claude ejecuta toda la ingeniería** (código, builds, verificación, documentación). Consecuencias prácticas:

- Las estimaciones en "sesiones" de los hitos pasan a ser **techo de esfuerzo humano equivalente**, no ritmo esperado: los hitos se ejecutan en ráfagas de agente y el objetivo es tener el POC **en días**.
- **Bucle de verificación en dos patas**: en el entorno remoto se verifica todo lo verificable sin GPU ni pantalla (compilación C++, tests headless de lógica y parsing, servidor completo); lo visual y el feel se prueban en la máquina del usuario, con comandos exactos incluidos en cada entrega.
- **Cada hito termina en un commit pusheado** con instrucciones de ejecución y una lista corta de "qué mirar". El feedback útil del usuario es: pegar el error de compilación tal cual, o describir/capturar lo que ve.
- Toolchain autocontenida: el build C++ solo requiere CMake ≥ 3.24 y un compilador; las dependencias (SDL3, ImGui, glslang como compilador de shaders, etc.) las descarga y compila el propio build con versiones fijadas.

## Hitos

Cada hito termina en algo ejecutable y demostrable. Estimaciones en sesiones de ~2–4 h equivalentes (ver Modo de ejecución).

### M0′ — Toolchain y triángulo (3–5 sesiones)

CMake + FetchContent (SDL3, GLM, cgltf, stb, ImGui) compilando en Windows y Linux. GPU device + swapchain con clear; paso de build que compila GLSL → SPIR-V (`glslc`); primer pipeline gráfico y triángulo en pantalla; ImGui integrado con contador de FPS. Estructura de carpetas de `docs/02` y estilo C++ decididos.

✅ *Hecho cuando*: `cmake --build` produce un binario que muestra el triángulo y el overlay de ImGui a 60 fps en ambas plataformas.

### M1′ — Coche conducible en local (4–6 sesiones)

Carga de glTF con cgltf y render de mallas con transform (modelo CC0 de coche y suelo de referencia con grid); cámara aérea con seguimiento suavizado; port provisional de `stepCar()` a C++ ejecutado en local (sin red; es el mismo modelo arcade del diseño); input de teclado; generador `npm run gen:constants` para compartir parámetros con `shared/`.

✅ *Hecho cuando*: conducir 2 minutos por el plano resulta satisfactorio. (Subjetivo a propósito: pilar 1.)

### M2′ — El look (4–6 sesiones)

El hito artístico: pase principal con luz direccional + niebla por distancia; tonemap + bloom sobre render targets; sombras (shadow map básico; si consume más de 2 sesiones → blob shadows y a otra cosa); primer escenario compuesto en Blender con assets CC0 y exportado por la convención `col_*`/`zone_*`/`spawn_*` (aún solo visual en el cliente).

✅ *Hecho cuando*: una captura del escenario con el coche transmite la atmósfera buscada (referencia RoR2) y la enseñarías sin disculparte.

### M3′ — Spike de red + servidor en el bucle (3–5 sesiones)

Primero el **spike** (1 sesión, timebox): validar el SDK nativo de Colyseus contra nuestro servidor 0.17 — join, recibir estado, enviar mensaje. Si falla, bajar por la escalera de D10 (mensajes msgpack/JSON o WS plano) y documentar. Después: el cliente envía `input {seq,...}`, el servidor simula con su `stepCar()` TS y publica snapshots; el cliente deja de simular y pasa a **interpolar**; el servidor carga la colisión y zonas del glTF del nivel. Latencia artificial configurable y overlay de debug (posición autoritativa vs presentada).

✅ *Hecho cuando*: con 100–150 ms de latencia artificial el coche sigue siendo controlable y manipular el cliente no mueve al coche del servidor. Si el feel con interpolación pura no convence, se anota y la predicción (D11) se decide como primer trabajo post-POC — no se improvisa dentro del hito.

### M4′ — Mundo persistente e interacciones (2–3 sesiones)

Identidad anónima persistida en el cliente; `Storage` JSON en servidor: posición/estado al desconectar, restauración al volver. Una zona `zone_*` del nivel evaluada en el servidor (checkpoint con vueltas o monedas) con `event` al cliente mostrado en el HUD.

✅ *Hecho cuando*: recoges monedas / marcas vueltas, cierras el juego, vuelves, y posición y progreso siguen ahí.

### M5′ — Cierre del POC (1–2 sesiones)

README con build e instrucciones en frío (máquina limpia), parámetros en `shared/` + header generado, pasada de feel, HUD limpio. Retirada del cliente web sonda si ya no aporta.

✅ *Hecho cuando*: una persona nueva clona el repo y juega el POC en menos de 15 minutos siguiendo el README (compilación incluida).

**Total estimado: 17–27 sesiones** (el plan 2D/web estimaba 11–17; raylib estimaba 14–22; la diferencia es el precio consciente del renderer propio sobre un API moderno sin pilas incluidas).

## Orden y dependencias

```
M0′ ──► M1′ ──► M2′ ──► M3′ ──► M4′ ──► M5′
                 ▲
                 └── el spike de red (inicio de M3′) puede adelantarse en paralelo
                     si hay ganas de despejar el riesgo antes
```

M1′ (feel local) sigue yendo antes que la red, por la misma razón del plan original: el feel se busca con iteración instantánea, no a través de la latencia. M2′ va antes que la red deliberadamente: es la validación del motivo del pivote — si el look no llega, mejor saberlo con el mínimo invertido.

## Riesgos

| # | Riesgo | Impacto | Mitigación |
|---|---|---|---|
| R1 | **Scope creep de motor** ("una feature de render más") | Alto — es el riesgo nº 1 | Lista de render cerrada en Alcance; backlog para todo lo demás; M2′ tiene criterio de salida concreto |
| R2 | SDK nativo de Colyseus inmaduro o incompatible con 0.17 | Medio | Spike timeboxed al inicio de M3′; escalera de fallbacks (D10) que conserva el servidor |
| R3 | Divergencia entre `stepCar` TS (servidor) y el port C++ local de M1′ | Medio | En M3′ el servidor pasa a mandar y el port local deja de simular; constantes generadas desde `shared/`; la predicción (que reabriría el riesgo) queda post-POC con estrategia D11 |
| R4 | Shadow mapping se come el calendario | Medio | Timebox de 2 sesiones; blob shadows como salida digna prevista |
| R5 | El feel con interpolación pura no convence (sin predicción) | Medio | Latencia artificial desde M3′ para medirlo pronto; decisión D11 preparada; en single-MMO local la latencia real es mínima |
| R6 | SDL3 GPU es un API joven (estable desde 2025): menos tutoriales y respuestas hechas que OpenGL/Vulkan | Medio | Repo oficial de ejemplos de SDL_gpu, backend ImGui como referencia de integración, documentación de SDL3; la lista cerrada de render limita la superficie expuesta |
| R7 | Pipeline Blender→glTF con fricción (exportes, convención) | Bajo | Convención mínima de 3 prefijos; validador en el servidor que lista lo que encontró al cargar el nivel |
| R8 | Builds C++ multiplataforma (dependencias, CMake, glslc en PATH) | Bajo | FetchContent con versiones fijadas; glslc puede venir del SDK de Vulkan o como binario del repo de shaderc; CI post-POC |

## Después del POC (no comprometido, para contexto)

Predicción local (D11) si el feel lo pide; diseño de juego (qué se hace en este mundo); dirección de arte propia; desniveles reales (heightmap); cross-compile de shaders y macOS si interesa; login real y despliegue del servidor; decisión informada MMO / single player / ambos; y el backlog de render acumulado, ya con un juego debajo que lo justifique.
