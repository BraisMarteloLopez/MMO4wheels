#pragma once

#include "render/mesh.h"

namespace m4w {

// Plano de suelo centrado en el origen, en Y = 0 (la rejilla la pinta el
// shader de suelo a partir de la posición de mundo).
MeshData makeGroundPlane(float half_extent);

} // namespace m4w
