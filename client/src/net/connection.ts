import { Client, type Room } from "@colyseus/sdk";
import {
  DEFAULT_SERVER_PORT,
  Msg,
  WORLD_ROOM,
  type PingPayload,
} from "@mmo4wheels/shared";

export type ConnectionStatus = "connecting" | "connected" | "disconnected";

export interface ConnectionEvents {
  status: ConnectionStatus;
  ping: number;
  tick: number;
}

type Listener<T> = (value: T) => void;

/** Emisor de eventos mínimo y tipado, sin dependencias de framework. */
class Emitter<Events> {
  private listeners: { [K in keyof Events]?: Set<Listener<Events[K]>> } = {};

  on<K extends keyof Events>(event: K, fn: Listener<Events[K]>): () => void {
    (this.listeners[event] ??= new Set()).add(fn);
    return () => this.listeners[event]?.delete(fn);
  }

  emit<K extends keyof Events>(event: K, value: Events[K]): void {
    this.listeners[event]?.forEach((fn) => fn(value));
  }
}

const PING_INTERVAL_MS = 2000;
const RECONNECT_DELAY_MS = 3000;

/**
 * Conexión con el servidor de juego. Gestiona el ciclo de vida de la sala
 * (join, pérdida de conexión, reintentos) y publica eventos para la UI.
 * La capa de red no conoce Phaser: las escenas se suscriben a `events`.
 */
export class Connection {
  readonly events = new Emitter<ConnectionEvents>();

  status: ConnectionStatus = "disconnected";
  /** Último RTT medido en ms (−1 = sin medir todavía). */
  ping = -1;

  private client: Client;
  private room?: Room;
  private pingTimer?: ReturnType<typeof setInterval>;

  constructor(private url: string = defaultServerUrl()) {
    this.client = new Client(this.url);
  }

  async connect(): Promise<void> {
    this.setStatus("connecting");
    try {
      const room = await this.client.joinOrCreate(WORLD_ROOM);
      this.room = room;
      this.setStatus("connected");

      room.onStateChange((state: { tick?: number }) => {
        this.events.emit("tick", state.tick ?? 0);
      });

      room.onMessage(Msg.Pong, (payload: PingPayload) => {
        this.ping = Math.round(performance.now() - payload.sentAt);
        this.events.emit("ping", this.ping);
      });

      room.onLeave(() => this.handleDisconnect());

      this.pingTimer = setInterval(() => {
        room.send(Msg.Ping, { sentAt: performance.now() } satisfies PingPayload);
      }, PING_INTERVAL_MS);
    } catch (err) {
      console.error("[net] error al conectar:", err);
      this.handleDisconnect();
    }
  }

  private handleDisconnect(): void {
    if (this.pingTimer !== undefined) clearInterval(this.pingTimer);
    this.pingTimer = undefined;
    this.room = undefined;
    this.setStatus("disconnected");
    setTimeout(() => void this.connect(), RECONNECT_DELAY_MS);
  }

  private setStatus(status: ConnectionStatus): void {
    this.status = status;
    this.events.emit("status", status);
  }
}

function defaultServerUrl(): string {
  return (
    (import.meta.env.VITE_SERVER_URL as string | undefined) ??
    `ws://${location.hostname}:${DEFAULT_SERVER_PORT}`
  );
}
