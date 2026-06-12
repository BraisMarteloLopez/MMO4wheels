/**
 * Smoke de la física TS con el mismo escenario que el selftest del cliente
 * C++ (client-native/src/platform/app.cpp): 10 s a fondo + 1 s girando.
 * Sirve para vigilar la paridad de los dos ports (D11).
 *
 *   npx tsx shared/scripts/physics-smoke.ts
 */
import { stepCar, type CarState } from "../src/physics";

const state: CarState = { x: 0, y: 0, heading: 0, vx: 0, vy: 0 };

for (let i = 0; i < 600; i++) {
  stepCar(state, { throttle: 1, steer: 0, brake: false }, 1 / 60);
}
const v = Math.hypot(state.vx, state.vy);
console.log(`recta: x=${state.x.toFixed(1)} m, v=${v.toFixed(2)} m/s`);

for (let i = 0; i < 60; i++) {
  stepCar(state, { throttle: 1, steer: 1, brake: false }, 1 / 60);
}
console.log(`giro: heading=${state.heading.toFixed(3)} rad`);

const ok = state.x > 150 && v < 36.01 && state.heading > 0.1;
console.log(ok ? "✓ física TS ok" : "✗ física TS fuera de rango");
process.exit(ok ? 0 : 1);
