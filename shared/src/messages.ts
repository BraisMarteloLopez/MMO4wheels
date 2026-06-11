/** Nombres de los mensajes intercambiados entre cliente y servidor. */
export const Msg = {
  /** C → S: medición de latencia. El servidor responde con `Pong` y el mismo payload. */
  Ping: "ping",
  /** S → C: respuesta a `Ping`. */
  Pong: "pong",
} as const;

/** Payload de `Msg.Ping` y `Msg.Pong`. */
export interface PingPayload {
  /** Marca de tiempo del cliente (`performance.now()`) al enviar el ping. */
  sentAt: number;
}
