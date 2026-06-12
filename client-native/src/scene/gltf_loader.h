#pragma once

#include "render/mesh.h"

namespace m4w {

// Carga un glTF y aplana todas sus mallas (con los transforms de nodo
// horneados en los vértices) en una única MeshData. Suficiente para objetos
// rígidos del POC; las jerarquías animadas llegarán más adelante.
bool loadGltfMesh(const char* path, MeshData& out);

} // namespace m4w
