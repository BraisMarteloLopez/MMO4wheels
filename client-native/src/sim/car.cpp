#include "sim/car.h"

#include <algorithm>
#include <cmath>

#include "generated/constants.h"

namespace m4w {

namespace {
inline float clampf(float v, float lo, float hi) { return std::min(hi, std::max(lo, v)); }
inline float signf(float v) { return v > 0.0f ? 1.0f : (v < 0.0f ? -1.0f : 0.0f); }
} // namespace

CarParams defaultCarParams() {
    return CarParams{
        .accel = gen::kCarAccel,
        .reverse_accel = gen::kCarReverseAccel,
        .max_speed = gen::kCarMaxSpeed,
        .max_reverse_speed = gen::kCarMaxReverseSpeed,
        .brake_decel = gen::kCarBrakeDecel,
        .roll_decel = gen::kCarRollDecel,
        .drag_q = gen::kCarDragQ,
        .steer_rate = gen::kCarSteerRate,
        .steer_ref_speed = gen::kCarSteerRefSpeed,
        .grip = gen::kCarGrip,
    };
}

void stepCar(CarState& state, const CarInput& input, float dt, const CarParams& p) {
    const float throttle = clampf(input.throttle, -1.0f, 1.0f);
    const float steer = clampf(input.steer, -1.0f, 1.0f);

    const float fx = std::cos(state.heading);
    const float fy = std::sin(state.heading);
    // perpendicular a la derecha del rumbo
    const float rx = -fy;
    const float ry = fx;

    // descomposición de la velocidad en el sistema del coche
    float v_forward = state.vx * fx + state.vy * fy;
    float v_lateral = state.vx * rx + state.vy * ry;

    // tracción
    v_forward += throttle * (throttle >= 0.0f ? p.accel : p.reverse_accel) * dt;

    // freno: deceleración hacia cero, sin invertir el sentido
    if (input.brake) {
        const float dec = std::min(std::abs(v_forward), p.brake_decel * dt);
        v_forward -= signf(v_forward) * dec;
    }

    // resistencias: rodadura constante + arrastre cuadrático
    const float resist = (p.roll_decel + p.drag_q * v_forward * v_forward) * dt;
    v_forward -= signf(v_forward) * std::min(std::abs(v_forward), resist);

    // agarre lateral: lo que no se amortigua es el derrape
    v_lateral *= std::exp(-p.grip * dt);

    v_forward = clampf(v_forward, -p.max_reverse_speed, p.max_speed);

    // giro: necesita velocidad para morder, y se invierte marcha atrás
    const float steer_factor = clampf(std::abs(v_forward) / p.steer_ref_speed, 0.0f, 1.0f);
    state.heading += steer * p.steer_rate * steer_factor * signf(v_forward) * dt;

    // recomposición y avance
    state.vx = fx * v_forward + rx * v_lateral;
    state.vy = fy * v_forward + ry * v_lateral;
    state.x += state.vx * dt;
    state.y += state.vy * dt;
}

} // namespace m4w
