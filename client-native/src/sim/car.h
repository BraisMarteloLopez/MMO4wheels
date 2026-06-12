#pragma once

namespace m4w {

// Estado del coche en el plano de simulación (x, y); heading 0 = +x, crece
// hacia +y. Mapeo a mundo 3D: (x, 0, y) con Y arriba.
struct CarState {
    float x = 0.0f;
    float y = 0.0f;
    float heading = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
};

struct CarInput {
    float throttle = 0.0f;  // -1..1
    float steer = 0.0f;     // -1..1
    bool brake = false;
};

struct CarParams {
    float accel;
    float reverse_accel;
    float max_speed;
    float max_reverse_speed;
    float brake_decel;
    float roll_decel;
    float drag_q;
    float steer_rate;
    float steer_ref_speed;
    float grip;
};

// Valores de shared/ (vía header generado).
CarParams defaultCarParams();

// Física arcade en planta. MANTENER EN ESPEJO con shared/src/physics.ts.
void stepCar(CarState& state, const CarInput& input, float dt, const CarParams& p);

} // namespace m4w
