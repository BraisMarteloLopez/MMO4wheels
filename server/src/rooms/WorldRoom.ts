import { Room, type Client } from "colyseus";
import { Schema, type } from "@colyseus/schema";
import { Msg, TICK_MS, type PingPayload } from "@mmo4wheels/shared";

export class WorldState extends Schema {
  /** Ticks simulados desde que existe el mundo. */
  @type("number") tick = 0;
}

/**
 * Sala única del mundo. Todos los clientes del POC entran aquí y el servidor
 * simula a tick fijo (TICK_RATE). En M0 el estado solo lleva el contador de
 * ticks para demostrar la sincronización; coche y física llegan en M1/M2.
 */
export class WorldRoom extends Room<{ state: WorldState }> {
  state = new WorldState();

  onCreate() {
    this.setSimulationInterval(() => this.update(), TICK_MS);

    this.onMessage(Msg.Ping, (client, payload: PingPayload) => {
      client.send(Msg.Pong, payload);
    });

    console.log(`[world] sala creada (${this.roomId})`);
  }

  update() {
    this.state.tick++;
  }

  onJoin(client: Client) {
    console.log(`[world] ${client.sessionId} conectado`);
  }

  onLeave(client: Client) {
    console.log(`[world] ${client.sessionId} desconectado`);
  }
}
