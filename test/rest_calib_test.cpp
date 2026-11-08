// Host-Test der Ruhekalibrierung: g++ -std=c++17 -Itest/stub test/rest_calib_test.cpp && ./a.out
#include <cassert>
#include <cmath>
#include <cstdio>
#include "../BLE/Bluetooth/SkiSenseMath.h"

static bool near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

int main() {
  // Ruhig, leicht geneigt, |a| = 1.02 g, Gyro-Bias (0.5, -1.2, 0.3), kleines Rauschen
  RestCalib c;
  const float ax = 0.0f, ay = 0.6f * 1.02f, az = 0.8f * 1.02f;
  for (int i = 0; i < 400; i++) {
    float e = (i % 2 ? 0.002f : -0.002f);
    c.add(ax + e, ay - e, az + e, 0.5f + 10 * e, -1.2f - 10 * e, 0.3f);
  }
  assert(c.ok());
  float accOff[3], gyrOff[3];
  c.offsets(accOff, gyrOff);
  assert(near(gyrOff[0], 0.5f) && near(gyrOff[1], -1.2f) && near(gyrOff[2], 0.3f));
  // Korrigiert: |a| = 1 g, Neigung (Richtung) unverändert -> Schwerkraft bleibt
  float cx = ax - accOff[0], cy = ay - accOff[1], cz = az - accOff[2];
  assert(near(norm3({cx, cy, cz}), 1.0f));
  assert(near(cy, 0.6f) && near(cz, 0.8f) && near(cx, 0.0f));

  // Bewegung (Gyro-Streuung) -> abgelehnt
  RestCalib moving;
  for (int i = 0; i < 400; i++) moving.add(0, 0, 1, (i % 2 ? 20.0f : -20.0f), 0, 0);
  assert(!moving.ok());

  // Freier Fall / falsche Einheit (kein ~1 g) -> abgelehnt
  RestCalib noG;
  for (int i = 0; i < 400; i++) noG.add(0, 0, 0.3f, 0, 0, 0);
  assert(!noG.ok());

  // Zu wenige Samples -> abgelehnt
  RestCalib few;
  for (int i = 0; i < 10; i++) few.add(0, 0, 1, 0, 0, 0);
  assert(!few.ok());

  std::puts("rest_calib_test: OK");
  return 0;
}
