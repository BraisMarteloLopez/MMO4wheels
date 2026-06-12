/**
 * Genera client-native/src/generated/constants.h a partir de las constantes
 * de shared/, para que el cliente C++ y el servidor TS no diverjan.
 *
 * Desde la raíz del repo:  npm run gen:constants
 */
import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";

import { CAR, INTERPOLATION_DELAY_MS, TICK_RATE } from "../src/constants";

const OUT = resolve(__dirname, "../../client-native/src/generated/constants.h");

const pascalToK = (name: string) =>
  "kCar" +
  name
    .toLowerCase()
    .split("_")
    .map((w) => w.charAt(0).toUpperCase() + w.slice(1))
    .join("");

const lines: string[] = [
  "// GENERADO por shared/scripts/gen-constants.ts — NO EDITAR A MANO.",
  "// Regenerar con: npm run gen:constants",
  "#pragma once",
  "",
  "namespace m4w::gen {",
  "",
  `inline constexpr float kTickRate = ${TICK_RATE.toFixed(1)}f;`,
  `inline constexpr float kInterpolationDelayMs = ${INTERPOLATION_DELAY_MS.toFixed(1)}f;`,
  "",
  "// Física arcade del coche (ver shared/src/constants.ts y physics.ts)",
];

for (const [key, value] of Object.entries(CAR)) {
  lines.push(`inline constexpr float ${pascalToK(key)} = ${value.toFixed(4)}f;`);
}

lines.push("", "} // namespace m4w::gen", "");

mkdirSync(dirname(OUT), { recursive: true });
writeFileSync(OUT, lines.join("\n"));
console.log(`✓ generado ${OUT}`);
