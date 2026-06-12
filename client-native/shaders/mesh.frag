#version 450

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec3 v_world_pos;

// Convención SDL_GPU para SPIR-V: UBOs de fragmento en set = 3
layout(set = 3, binding = 0) uniform FragUbo {
    vec4 u_sun_dir;    // xyz: dirección HACIA la luz (normalizada)
    vec4 u_sun_color;  // rgb: color, a: intensidad
    vec4 u_ambient;    // rgb: luz ambiente
};

layout(location = 0) out vec4 o_color;

void main() {
    vec3 n = normalize(v_normal);
    float ndl = max(dot(n, normalize(u_sun_dir.xyz)), 0.0);
    vec3 lit = v_color.rgb * (u_ambient.rgb + u_sun_color.rgb * (u_sun_color.a * ndl));
    o_color = vec4(lit, v_color.a);
}
