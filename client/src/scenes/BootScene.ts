import Phaser from "phaser";

export class BootScene extends Phaser.Scene {
  constructor() {
    super("boot");
  }

  create() {
    this.createCarTexture();
    this.scene.start("game");
  }

  /** Coche placeholder de 12×20 px dibujado a mano, hasta tener assets reales (M1). */
  private createCarTexture(): void {
    const canvas = this.textures.createCanvas("car", 12, 20);
    if (!canvas) throw new Error("No se pudo crear la textura del coche");
    const ctx = canvas.getContext();

    // ruedas
    ctx.fillStyle = "#111111";
    ctx.fillRect(0, 2, 2, 5);
    ctx.fillRect(10, 2, 2, 5);
    ctx.fillRect(0, 13, 2, 5);
    ctx.fillRect(10, 13, 2, 5);

    // carrocería
    ctx.fillStyle = "#d83b3b";
    ctx.fillRect(1, 0, 10, 20);
    ctx.fillStyle = "#a52a2a";
    ctx.fillRect(1, 0, 10, 2);
    ctx.fillRect(1, 18, 10, 2);

    // parabrisas y luneta
    ctx.fillStyle = "#9fd3e6";
    ctx.fillRect(2, 4, 8, 3);
    ctx.fillRect(3, 14, 6, 2);

    canvas.refresh();
  }
}
