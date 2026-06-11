import Phaser from "phaser";

export class GameScene extends Phaser.Scene {
  constructor() {
    super("game");
  }

  create() {
    const { width, height } = this.scale;

    this.drawGrid();
    this.add.image(width / 2, height / 2, "car").setScale(4);

    this.add
      .text(width / 2, height - 24, "M0 — esqueleto conectado", {
        fontFamily: "monospace",
        fontSize: "12px",
        color: "#5c6b8a",
      })
      .setOrigin(0.5);

    this.scene.launch("hud");
  }

  private drawGrid(): void {
    const { width, height } = this.scale;
    const cell = 32;
    const g = this.add.graphics();
    g.lineStyle(1, 0x1f2a44, 1);
    for (let x = 0; x <= width; x += cell) g.lineBetween(x, 0, x, height);
    for (let y = 0; y <= height; y += cell) g.lineBetween(0, y, width, y);
  }
}
