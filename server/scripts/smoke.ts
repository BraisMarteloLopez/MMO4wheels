/**
 * Prueba de humo de conectividad: se une a la sala del mundo, espera a que el
 * estado sincronice y mide un ping. Requiere el servidor levantado.
 *
 * Desde la raíz del repo:  npm run smoke
 */
import { Client } from "@colyseus/sdk";
import {
  DEFAULT_SERVER_PORT,
  Msg,
  WORLD_ROOM,
  type PingPayload,
} from "@mmo4wheels/shared";

const url = process.env.SERVER_URL ?? `ws://localhost:${DEFAULT_SERVER_PORT}`;
const TIMEOUT_MS = 10_000;

async function main(): Promise<void> {
  const timeout = setTimeout(() => {
    console.error(`✗ timeout: sin respuesta del servidor en ${TIMEOUT_MS} ms`);
    process.exit(1);
  }, TIMEOUT_MS);

  console.log(`→ conectando a ${url}…`);
  const client = new Client(url);
  const room = await client.joinOrCreate(WORLD_ROOM);
  console.log(`✓ unido a la sala (sessionId ${room.sessionId})`);

  const tick = await new Promise<number>((resolve) => {
    room.onStateChange((state: { tick?: number }) => {
      const t = state.tick ?? 0;
      if (t > 0) resolve(t);
    });
  });
  console.log(`✓ estado sincronizado (tick ${tick})`);

  const rtt = await new Promise<number>((resolve) => {
    room.onMessage(Msg.Pong, (p: PingPayload) => resolve(performance.now() - p.sentAt));
    room.send(Msg.Ping, { sentAt: performance.now() } satisfies PingPayload);
  });
  console.log(`✓ ping/pong ok (rtt ${rtt.toFixed(1)} ms)`);

  await room.leave();
  clearTimeout(timeout);
  console.log("✓ smoke test superado");
  process.exit(0);
}

main().catch((err) => {
  console.error("✗ smoke test falló:", err);
  process.exit(1);
});
