#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace m4w {

struct Camera {
    glm::vec3 position{0.0f, 14.0f, 10.0f};
    glm::vec3 target{0.0f};
    float fov_deg = 50.0f;
    float near_plane = 0.1f;
    float far_plane = 600.0f;

    glm::mat4 view() const {
        return glm::lookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 proj(float aspect) const {
        return glm::perspective(glm::radians(fov_deg), aspect, near_plane, far_plane);
    }
};

} // namespace m4w
