// Genera assets/models/car.gltf: coche low-poly placeholder hecho de cajas
// con colores por vértice. Sustituible por un asset CC0 sin tocar el cliente.
//
//   node assets/tools/gen-car.mjs
//
// Convenciones: el coche apunta a +X, Y es arriba, Z es la anchura.
// Unidades en metros (~3.9 m de largo).

import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const OUT = resolve(dirname(fileURLToPath(import.meta.url)), "../models/car.gltf");

const positions = [];
const normals = [];
const colors = [];
const indices = [];

function addBox([cx, cy, cz], [sx, sy, sz], [r, g, b]) {
  const x = sx / 2, y = sy / 2, z = sz / 2;
  // por cara: 4 vértices con su normal (orden CCW visto desde fuera)
  const faces = [
    { n: [1, 0, 0],  v: [[x, -y, -z], [x, y, -z], [x, y, z], [x, -y, z]] },
    { n: [-1, 0, 0], v: [[-x, -y, z], [-x, y, z], [-x, y, -z], [-x, -y, -z]] },
    { n: [0, 1, 0],  v: [[-x, y, -z], [-x, y, z], [x, y, z], [x, y, -z]] },
    { n: [0, -1, 0], v: [[-x, -y, z], [-x, -y, -z], [x, -y, -z], [x, -y, z]] },
    { n: [0, 0, 1],  v: [[-x, -y, z], [x, -y, z], [x, y, z], [-x, y, z]] },
    { n: [0, 0, -1], v: [[x, -y, -z], [-x, -y, -z], [-x, y, -z], [x, y, -z]] },
  ];
  for (const { n, v } of faces) {
    const base = positions.length / 3;
    for (const [vx, vy, vz] of v) {
      positions.push(cx + vx, cy + vy, cz + vz);
      normals.push(...n);
      colors.push(r, g, b, 1);
    }
    indices.push(base, base + 1, base + 2, base, base + 2, base + 3);
  }
}

const BODY = [0.82, 0.26, 0.22];
const GLASS = [0.16, 0.2, 0.28];
const DARK = [0.11, 0.11, 0.12];
const WHEEL = [0.07, 0.07, 0.08];

addBox([0, 0.65, 0], [3.9, 0.85, 1.75], BODY);          // carrocería
addBox([-0.35, 1.25, 0], [1.7, 0.55, 1.5], GLASS);       // cabina
addBox([1.95, 0.45, 0], [0.25, 0.45, 1.6], DARK);        // parachoques delantero
addBox([-1.95, 0.45, 0], [0.25, 0.45, 1.6], DARK);       // parachoques trasero
for (const fx of [1.25, -1.25]) {
  for (const fz of [0.93, -0.93]) {
    addBox([fx, 0.36, fz], [0.72, 0.72, 0.32], WHEEL);   // ruedas
  }
}

// --- empaquetado glTF ---

const posArr = new Float32Array(positions);
const nrmArr = new Float32Array(normals);
const colArr = new Float32Array(colors);
const idxArr = new Uint16Array(indices);

const align = (n, a) => Math.ceil(n / a) * a;
const posOff = 0;
const nrmOff = align(posOff + posArr.byteLength, 4);
const colOff = align(nrmOff + nrmArr.byteLength, 4);
const idxOff = align(colOff + colArr.byteLength, 4);
const total = idxOff + idxArr.byteLength;

const buf = new Uint8Array(total);
buf.set(new Uint8Array(posArr.buffer), posOff);
buf.set(new Uint8Array(nrmArr.buffer), nrmOff);
buf.set(new Uint8Array(colArr.buffer), colOff);
buf.set(new Uint8Array(idxArr.buffer), idxOff);

const vertexCount = positions.length / 3;
const min = [Infinity, Infinity, Infinity];
const max = [-Infinity, -Infinity, -Infinity];
for (let i = 0; i < positions.length; i += 3) {
  for (let k = 0; k < 3; k++) {
    min[k] = Math.min(min[k], positions[i + k]);
    max[k] = Math.max(max[k], positions[i + k]);
  }
}

const gltf = {
  asset: { version: "2.0", generator: "mmo4wheels gen-car" },
  scene: 0,
  scenes: [{ nodes: [0] }],
  nodes: [{ mesh: 0, name: "car" }],
  meshes: [
    {
      name: "car",
      primitives: [
        { attributes: { POSITION: 0, NORMAL: 1, COLOR_0: 2 }, indices: 3, mode: 4 },
      ],
    },
  ],
  accessors: [
    { bufferView: 0, componentType: 5126, count: vertexCount, type: "VEC3", min, max },
    { bufferView: 1, componentType: 5126, count: vertexCount, type: "VEC3" },
    { bufferView: 2, componentType: 5126, count: vertexCount, type: "VEC4" },
    { bufferView: 3, componentType: 5123, count: indices.length, type: "SCALAR" },
  ],
  bufferViews: [
    { buffer: 0, byteOffset: posOff, byteLength: posArr.byteLength, target: 34962 },
    { buffer: 0, byteOffset: nrmOff, byteLength: nrmArr.byteLength, target: 34962 },
    { buffer: 0, byteOffset: colOff, byteLength: colArr.byteLength, target: 34962 },
    { buffer: 0, byteOffset: idxOff, byteLength: idxArr.byteLength, target: 34963 },
  ],
  buffers: [
    {
      byteLength: total,
      uri: "data:application/octet-stream;base64," + Buffer.from(buf).toString("base64"),
    },
  ],
};

mkdirSync(dirname(OUT), { recursive: true });
writeFileSync(OUT, JSON.stringify(gltf, null, 2));
console.log(`✓ generado ${OUT} (${vertexCount} vértices, ${indices.length / 3} triángulos)`);
