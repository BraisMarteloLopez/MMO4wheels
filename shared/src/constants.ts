/** Ticks de simulación del servidor por segundo. */
export const TICK_RATE = 20;

/** Milisegundos por tick de simulación. */
export const TICK_MS = 1000 / TICK_RATE;

/** Nombre de la sala principal del mundo. */
export const WORLD_ROOM = "world";

/** Puerto por defecto del servidor de juego. */
export const DEFAULT_SERVER_PORT = 2567;

/** Retraso de presentación del cliente al interpolar snapshots, en ms. */
export const INTERPOLATION_DELAY_MS = 100;

/**
 * Parámetros de la física arcade del coche (unidades SI: metros, segundos,
 * radianes). Fuente de verdad compartida: el servidor los usa desde aquí y
 * `npm run gen:constants` los exporta a C++ (client-native/src/generated/).
 * El feel se tunea en vivo con los sliders del cliente y los valores buenos
 * se consolidan aquí.
 */
export const CAR = {
  /** Aceleración a fondo hacia delante, m/s². */
  ACCEL: 24,
  /** Aceleración marcha atrás, m/s². */
  REVERSE_ACCEL: 12,
  /** Velocidad máxima hacia delante, m/s (~130 km/h). */
  MAX_SPEED: 36,
  /** Velocidad máxima marcha atrás, m/s. */
  MAX_REVERSE_SPEED: 9,
  /** Deceleración de frenada, m/s². */
  BRAKE_DECEL: 38,
  /** Fricción de rodadura (deceleración constante), m/s². */
  ROLL_DECEL: 2.5,
  /** Coeficiente de arrastre cuadrático, 1/m (decel = DRAG_Q · v²). */
  DRAG_Q: 0.018,
  /** Velocidad de giro con agarre pleno, rad/s. */
  STEER_RATE: 2.4,
  /** Velocidad longitudinal a la que el giro alcanza su máximo, m/s. */
  STEER_REF_SPEED: 6,
  /** Amortiguación de la velocidad lateral, 1/s (más alto = menos derrape). */
  GRIP: 7,
} as const;
