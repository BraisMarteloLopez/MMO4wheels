#version 450

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;

layout(set = 1, binding = 0) uniform VertUbo {
    mat4 u_mvp;
    mat4 u_model;
};

layout(location = 0) out vec3 v_world_pos;

void main() {
    gl_Position = u_mvp * vec4(a_pos, 1.0);
    v_world_pos = (u_model * vec4(a_pos, 1.0)).xyz;
}
