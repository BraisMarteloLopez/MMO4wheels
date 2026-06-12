#include "scene/procedural.h"

namespace m4w {

MeshData makeGroundPlane(float half_extent) {
    const float h = half_extent;
    const glm::vec3 up{0.0f, 1.0f, 0.0f};
    const glm::vec4 white{1.0f};

    MeshData mesh;
    mesh.vertices = {
        {{-h, 0.0f, -h}, up, white},
        {{-h, 0.0f, h}, up, white},
        {{h, 0.0f, h}, up, white},
        {{h, 0.0f, -h}, up, white},
    };
    mesh.indices = {0, 1, 2, 0, 2, 3};
    return mesh;
}

} // namespace m4w
