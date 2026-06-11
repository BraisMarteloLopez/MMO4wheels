import Phaser from "phaser";
import { Connection } from "./net/connection";
import { BootScene } from "./scenes/BootScene";
import { GameScene } from "./scenes/GameScene";
import { HudScene } from "./scenes/HudScene";

const connection = new Connection();
void connection.connect();

const game = new Phaser.Game({
  type: Phaser.AUTO,
  parent: "app",
  width: 800,
  height: 600,
  pixelArt: true,
  backgroundColor: "#16213e",
  scene: [BootScene, GameScene, HudScene],
});

game.registry.set("connection", connection);
