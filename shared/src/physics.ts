import { CAR } from "./constants";

/**
 * Física arcade del coche en planta (2D): modelo compartido del proyecto.
 *
 * MANTENER EN ESPEJO con el port C++ del cliente
 * (client-native/src/sim/car.cpp): misma secuencia de operaciones, mismos
 * parámetros (generados desde constants.ts).
 */

export interface CarState {
  /** Posición en el plano, metros. */
  x: number;
  y: number;
  /** Rumbo en radianes; 0 = +x, crece hacia +y. */
  heading: number;
  /** Velocidad en coordenadas de mundo, m/s. */
  vx: number;
  vy: number;
}

export interface CarInput {
  /** Acelerador: -1 (marcha atrás) .. 1 (a fondo). */
  throttle: number;
  /** Dirección: -1 (izquierda) .. 1 (derecha). */
  steer: number;
  /** Freno. */
  brake: boolean;
}

export type CarParams = { -readonly [K in keyof typeof CAR]: number };

export const defaultCarParams = (): CarParams => ({ ...CAR });

const clamp = (v: number, lo: number, hi: number) => Math.min(hi, Math.max(lo, v));

/**
 * Avanza la simulación del coche un paso de `dt` segundos. Función pura
 * sobre `state` (lo muta y lo devuelve) y determinista para un mismo dt.
 */
export function stepCar(
  state: CarState,
  input: CarInput,
  dt: number,
  p: CarParams = CAR as CarParams,
): CarState {
  const throttle = clamp(input.throttle, -1, 1);
  const steer = clamp(input.steer, -1, 1);

  const fx = Math.cos(state.heading);
  const fy = Math.sin(state.heading);
  // perpendicular a la derecha del rumbo
  const rx = -fy;
  const ry = fx;

  // descomposición de la velocidad en el sistema del coche
  let vForward = state.vx * fx + state.vy * fy;
  let vLateral = state.vx * rx + state.vy * ry;

  // tracción
  vForward += throttle * (throttle >= 0 ? p.ACCEL : p.REVERSE_ACCEL) * dt;

  // freno: deceleración hacia cero, sin invertir el sentido
  if (input.brake) {
    const dec = Math.min(Math.abs(vForward), p.BRAKE_DECEL * dt);
    vForward -= Math.sign(vForward) * dec;
  }

  // resistencias: rodadura constante + arrastre cuadrático
  const resist = (p.ROLL_DECEL + p.DRAG_Q * vForward * vForward) * dt;
  vForward -= Math.sign(vForward) * Math.min(Math.abs(vForward), resist);

  // agarre lateral: lo que no se amortigua es el derrape
  vLateral *= Math.exp(-p.GRIP * dt);

  vForward = clamp(vForward, -p.MAX_REVERSE_SPEED, p.MAX_SPEED);

  // giro: necesita velocidad para morder, y se invierte marcha atrás
  const steerFactor = clamp(Math.abs(vForward) / p.STEER_REF_SPEED, 0, 1);
  state.heading += steer * p.STEER_RATE * steerFactor * Math.sign(vForward) * dt;

  // recomposición y avance
  state.vx = fx * vForward + rx * vLateral;
  state.vy = fy * vForward + ry * vLateral;
  state.x += state.vx * dt;
  state.y += state.vy * dt;

  return state;
}
