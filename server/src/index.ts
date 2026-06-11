import { Server, WebSocketTransport } from "colyseus";
import { DEFAULT_SERVER_PORT, WORLD_ROOM } from "@mmo4wheels/shared";
import { WorldRoom } from "./rooms/WorldRoom";

const port = Number(process.env.PORT ?? DEFAULT_SERVER_PORT);

const gameServer = new Server({
  transport: new WebSocketTransport(),
});

gameServer.define(WORLD_ROOM, WorldRoom);

void gameServer.listen(port).then(() => {
  console.log(`[server] MMO4wheels escuchando en ws://localhost:${port}`);
});
