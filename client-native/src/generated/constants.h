// GENERADO por shared/scripts/gen-constants.ts — NO EDITAR A MANO.
// Regenerar con: npm run gen:constants
#pragma once

namespace m4w::gen {

inline constexpr float kTickRate = 20.0f;
inline constexpr float kInterpolationDelayMs = 100.0f;

// Física arcade del coche (ver shared/src/constants.ts y physics.ts)
inline constexpr float kCarAccel = 24.0000f;
inline constexpr float kCarReverseAccel = 12.0000f;
inline constexpr float kCarMaxSpeed = 36.0000f;
inline constexpr float kCarMaxReverseSpeed = 9.0000f;
inline constexpr float kCarBrakeDecel = 38.0000f;
inline constexpr float kCarRollDecel = 2.5000f;
inline constexpr float kCarDragQ = 0.0180f;
inline constexpr float kCarSteerRate = 2.4000f;
inline constexpr float kCarSteerRefSpeed = 6.0000f;
inline constexpr float kCarGrip = 7.0000f;

} // namespace m4w::gen
