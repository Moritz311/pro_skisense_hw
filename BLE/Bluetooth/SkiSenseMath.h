#pragma once
#include <Arduino.h>
#include <math.h>

// ---- Vektoren & Matrizen ----
struct Vec3 { float x, y, z; };
struct Mat3 { float m[3][3]; };

// Euler ZYX (yaw->pitch->roll) in Rotationsmatrix
static inline Mat3 eulerZYX_deg(float yaw, float pitch, float roll) {
  float cy = cosf(yaw * PI / 180.0f), sy = sinf(yaw * PI / 180.0f);
  float cp = cosf(pitch * PI / 180.0f), sp = sinf(pitch * PI / 180.0f);
  float cr = cosf(roll * PI / 180.0f),  sr = sinf(roll * PI / 180.0f);
  Mat3 R;
  R.m[0][0] = cy*cp;              R.m[0][1] = cy*sp*sr - sy*cr;  R.m[0][2] = cy*sp*cr + sy*sr;
  R.m[1][0] = sy*cp;              R.m[1][1] = sy*sp*sr + cy*cr;  R.m[1][2] = sy*sp*cr - cy*sr;
  R.m[2][0] = -sp;                R.m[2][1] = cp*sr;             R.m[2][2] = cp*cr;
  return R;
}

// Matrix * Vektor
static inline Vec3 mul(const Mat3& R, const Vec3& v) {
  return {
    R.m[0][0]*v.x + R.m[0][1]*v.y + R.m[0][2]*v.z,
    R.m[1][0]*v.x + R.m[1][1]*v.y + R.m[1][2]*v.z,
    R.m[2][0]*v.x + R.m[2][1]*v.y + R.m[2][2]*v.z
  };
}

// ||v||
static inline float norm3(const Vec3& v) {
  return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}
