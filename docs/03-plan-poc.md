# Plan del POC

## Objetivo

Demostrar, con el mínimo juego posible, que la base de MMO4wheels funciona:

1. **Conducir es agradable** — coche top‑down en pixel art con feel arcade.
2. **El servidor es la autoridad** — el cliente no puede decidir su posición; con latencia el juego sigue siendo jugable.
3. **El mundo persiste** — cierras y vuelves: estás donde estabas, con tu progreso.
4. **La arquitectura sostiene los dos futuros** — multijugador (varias conexiones ya entran en la misma sala) y single player (servidor en local).

## Alcance

**Dentro**: un coche, un escenario (mapa Tiled con colisiones), conexión cliente‑servidor, simulación autoritativa a tick fijo, interpolación (+ predicción si hace falta para el feel), una zona de interacción gestionada por el servidor (checkpoint o recogida de monedas), persistencia simple por jugador, HUD mínimo (velocidad, ping, progreso) y overlay de debug.

**Fuera** (explícitamente, contra el scope creep): otros jugadores visibles*, sonido, menús, login real, arte propio, varios coches/escenarios, tráfico/NPCs, optimización de red, despliegue en producción, móvil.

\* Si se conectan dos pestañas, Colyseus meterá ambos coches en la sala y se verán "gratis" — bienvenido sea como demo, pero no es objetivo ni se pulirá.

## Hitos

Cada hito termina en algo ejecutable y demostrable. Estimaciones en sesiones de trabajo (~2–4 h), pensadas para una persona.

### M0 — Esqueleto conectado (2–3 sesiones)

Monorepo npm workspaces (`client/`, `server/`, `shared/`) con TypeScript configurado y versiones fijadas (Node LTS, Colyseus 0.16.x; evaluar Phaser 3 vs 4 estable aquí). Cliente Vite+Phaser mostrando un canvas con un sprite; servidor Colyseus con `WorldRoom` vacía; el cliente se conecta al arrancar.

✅ *Hecho cuando*: `npm run dev` levanta cliente y servidor, y el HUD muestra «conectado» con el ping.

### M1 — Conducir en local (3–4 sesiones)

`stepCar()` en `shared/` con sus parámetros (aceleración, fricción, giro, derrape). El cliente lo ejecuta localmente (aún sin servidor en el bucle): coche controlable con teclado, cámara que sigue, mapa Tiled con colisiones, assets CC0 (Kenney), escalado pixel‑perfect. Aquí se decide tamaño de tile (16 vs 32 px) y se tunea el feel.

✅ *Hecho cuando*: conducir 2 minutos por el escenario resulta satisfactorio sin tocar código. (Criterio subjetivo a propósito: es el pilar 1.)

### M2 — El servidor toma el control (3–5 sesiones)

El cliente deja de simular y pasa a enviar `input {seq, throttle, steer, brake}`. El servidor simula a 20 Hz con el mismo `stepCar()`, resuelve colisiones contra el mapa y publica el estado vía Schema. El cliente renderiza con interpolación (~100 ms). Herramienta de latencia artificial + overlay de debug (posición autoritativa vs. local).

**M2.5 (condicional)**: si con 100–150 ms de latencia artificial el control se siente mal —que se sentirá—, añadir predicción local + reconciliación por `lastProcessedSeq`. Se hace dentro del POC porque valida la pieza técnica más arriesgada del proyecto.

✅ *Hecho cuando*: con 150 ms de latencia artificial el coche es controlable, y modificar la posición desde la consola del navegador no sirve de nada (el servidor manda).

### M3 — Mundo persistente e interacciones (2–3 sesiones)

Identidad anónima (id en `localStorage`). Interfaz `Storage` con implementación JSON: al desconectar se guarda posición/estado; al volver, reapareces donde estabas. Una zona de interacción definida en el mapa (capa de objetos de Tiled) y evaluada **en el servidor**: checkpoint con contador de vueltas o monedas recogibles. El servidor notifica con un `event` que el HUD muestra.

✅ *Hecho cuando*: recoges monedas / marcas vueltas, cierras la pestaña, vuelves, y tu posición y progreso siguen ahí.

### M4 — Cierre del POC (1–2 sesiones)

README con instrucciones de ejecución en un comando, parámetros de juego extraídos a `shared/constants.ts`, pasada final de feel, HUD limpio (velocidad, ping, progreso). Opcional: deploy del servidor en un servicio gratuito + cliente estático, para demo por URL.

✅ *Hecho cuando*: una persona nueva clona el repo y juega el POC en menos de 5 minutos siguiendo el README.

**Total estimado: 11–17 sesiones de trabajo.**

## Orden y dependencias

```
M0 ──► M1 ──► M2 (─► M2.5) ──► M3 ──► M4
```

M1 va antes que M2 deliberadamente: primero se encuentra el feel sin red de por medio (iteración instantánea); meter el servidor en el bucle después es mover una física que ya gusta, no buscarla a través de la latencia.

## Riesgos

| # | Riesgo | Impacto | Mitigación |
|---|---|---|---|
| R1 | El feel se degrada con el servidor en el bucle (latencia) | Alto — es el pilar 1 | Física compartida + predicción (M2.5); latencia artificial desde M2 para no engañarse en local |
| R2 | Divergencia cliente/servidor en la predicción (floats, dt) | Medio | Misma función `stepCar` en ambos lados, `dt` fijo, reconciliación con corrección suave; overlay de debug para verla |
| R3 | Scope creep (es un "MMO", todo parece necesario) | Alto | Lista «fuera» explícita; cada hito cierra algo demostrable |
| R4 | Atascarse en arte | Medio | Assets CC0 para todo el POC; el arte propio es fase posterior |
| R5 | Versiones/ecosistema (Phaser 4, Colyseus 0.16) | Bajo | Fijar versiones en M0 y no actualizarlas durante el POC |

## Después del POC (no comprometido, para contexto)

Diseño de juego (qué se hace en este mundo: carreras, misiones, economía), arte propio y pipeline, login real, despliegue estable del servidor, área de interés si hay multijugador, y la decisión informada: ¿MMO, single player, o ambos?
