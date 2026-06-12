#version 450

layout(location = 0) out vec3 v_color;

const vec2 POSITIONS[3] = vec2[](
    vec2( 0.0,  0.6),
    vec2(-0.6, -0.6),
    vec2( 0.6, -0.6)
);

const vec3 COLORS[3] = vec3[](
    vec3(0.95, 0.35, 0.35),
    vec3(0.35, 0.95, 0.55),
    vec3(0.40, 0.55, 0.95)
);

void main() {
    gl_Position = vec4(POSITIONS[gl_VertexIndex], 0.0, 1.0);
    v_color = COLORS[gl_VertexIndex];
}
