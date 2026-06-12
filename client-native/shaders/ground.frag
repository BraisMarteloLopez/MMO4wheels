#version 450

layout(location = 0) in vec3 v_world_pos;

layout(set = 3, binding = 0) uniform FragUbo {
    vec4 u_sun_dir;
    vec4 u_sun_color;
    vec4 u_ambient;
};

layout(location = 0) out vec4 o_color;

// Línea de rejilla antialiasada en el plano XZ del mundo.
float gridLine(vec2 p, float cell) {
    vec2 q = p / cell;
    vec2 g = abs(fract(q - 0.5) - 0.5) / fwidth(q);
    return 1.0 - min(min(g.x, g.y), 1.0);
}

void main() {
    vec3 base = vec3(0.115, 0.13, 0.15);
    float fine = gridLine(v_world_pos.xz, 2.0) * 0.10;
    float coarse = gridLine(v_world_pos.xz, 20.0) * 0.20;
    vec3 col = base + vec3(fine + coarse);
    o_color = vec4(col, 1.0);
}
