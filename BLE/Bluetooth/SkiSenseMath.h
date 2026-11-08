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

// ---- Ruhekalibrierung: Mittelwert/Streuung roher Samples (acc in g, gyr in dps) ----
struct RestCalib {
  double sum[6] = {0}, sq[6] = {0};
  uint32_t n = 0;

  void add(float ax, float ay, float az, float gx, float gy, float gz) {
    const float v[6] = {ax, ay, az, gx, gy, gz};
    for (int i = 0; i < 6; i++) { sum[i] += v[i]; sq[i] += (double)v[i] * v[i]; }
    n++;
  }
  float mean(int i) const { return (float)(sum[i] / n); }
  float stddev(int i) const {
    double m = sum[i] / n, var = sq[i] / n - m * m;
    return var > 0 ? (float)sqrt(var) : 0.0f;
  }

  // Bewegt oder nicht ~1 g -> Ergebnis unbrauchbar
  bool ok() const {
    if (n < 100) return false;
    float g = norm3({mean(0), mean(1), mean(2)});
    if (g < 0.9f || g > 1.1f) return false;
    for (int i = 0; i < 3; i++)
      if (stddev(i) > 0.05f || stddev(3 + i) > 1.0f) return false;
    return true;
  }

  // Gyro-Bias = Mittelwert. Accel: aus einer Lage ist nur der Betragsfehler entlang
  // der Schwerkraft bestimmbar; die Richtung (Neigung) bleibt erhalten, |a| wird 1 g.
  void offsets(float accOff[3], float gyrOff[3]) const {
    Vec3 a = {mean(0), mean(1), mean(2)};
    float k = 1.0f - 1.0f / norm3(a);
    accOff[0] = a.x * k; accOff[1] = a.y * k; accOff[2] = a.z * k;
    for (int i = 0; i < 3; i++) gyrOff[i] = mean(3 + i);
  }
};
