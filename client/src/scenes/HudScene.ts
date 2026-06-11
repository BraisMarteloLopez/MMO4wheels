import Phaser from "phaser";
import type { Connection, ConnectionStatus } from "../net/connection";

const STATUS_LABEL: Record<ConnectionStatus, { text: string; color: string }> = {
  connecting: { text: "conectando…", color: "#ffd166" },
  connected: { text: "conectado", color: "#80ed99" },
  disconnected: { text: "sin conexión — reintentando…", color: "#e63946" },
};

export class HudScene extends Phaser.Scene {
  private statusText!: Phaser.GameObjects.Text;
  private pingText!: Phaser.GameObjects.Text;
  private tickText!: Phaser.GameObjects.Text;
  private unsubscribe: Array<() => void> = [];

  constructor() {
    super("hud");
  }

  create() {
    const style = { fontFamily: "monospace", fontSize: "14px" };
    this.statusText = this.add.text(8, 8, "", style);
    this.pingText = this.add.text(8, 28, "ping: — ms", { ...style, color: "#8ecae6" });
    this.tickText = this.add.text(8, 48, "tick servidor: —", { ...style, color: "#8ecae6" });

    const conn = this.registry.get("connection") as Connection;
    this.renderStatus(conn.status);
    this.unsubscribe.push(
      conn.events.on("status", (s) => this.renderStatus(s)),
      conn.events.on("ping", (ms) => this.pingText.setText(`ping: ${ms} ms`)),
      conn.events.on("tick", (t) => this.tickText.setText(`tick servidor: ${t}`)),
    );

    this.events.once(Phaser.Scenes.Events.SHUTDOWN, () => {
      this.unsubscribe.forEach((off) => off());
      this.unsubscribe = [];
    });
  }

  private renderStatus(status: ConnectionStatus): void {
    const { text, color } = STATUS_LABEL[status];
    this.statusText.setText(`servidor: ${text}`).setColor(color);
  }
}
