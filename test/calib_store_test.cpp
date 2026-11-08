// Host-Test der Offset-Persistenz: g++ -std=c++17 -Itest/stub test/calib_store_test.cpp && ./a.out
#include <cassert>
#include <cmath>
#include <cstdio>
#include "../BLE/Bluetooth/CalibStore.h"

int main() {
  const float aS[3] = {0.01f, -0.02f, 0.03f}, gS[3] = {0.5f, -1.2f, 0.3f};
  const float aR[3] = {-0.01f, 0.0f, 0.02f}, gR[3] = {0.1f, 0.2f, -0.3f};
  CalibBlob b, out{};
  calibPack(b, aS, gS, aR, gR);
  assert(calibUnpack(&b, sizeof b, out));
  assert(out.gyrShin[1] == -1.2f && out.accRist[2] == 0.02f && out.gyrRist[2] == -0.3f);

  assert(!calibUnpack(&b, sizeof b - 1, out));          // falsche Länge
  CalibBlob x = b; x.gyrShin[0] += 0.001f;
  assert(!calibUnpack(&x, sizeof x, out));              // CRC (Bitfehler)
  x = b; x.magic ^= 1; x.crc = calibCrc32(&x, offsetof(CalibBlob, crc));
  assert(!calibUnpack(&x, sizeof x, out));              // falsches Magic
  x = b; x.version++; x.crc = calibCrc32(&x, offsetof(CalibBlob, crc));
  assert(!calibUnpack(&x, sizeof x, out));              // andere Version
  x = b; x.accShin[0] = NAN; x.crc = calibCrc32(&x, offsetof(CalibBlob, crc));
  assert(!calibUnpack(&x, sizeof x, out));              // NaN trotz gültiger CRC
  char blank[sizeof(CalibBlob)]; memset(blank, 0xFF, sizeof blank);
  assert(!calibUnpack(blank, sizeof blank, out));       // gelöschter Flash
  assert(calibCrc32("123456789", 9) == 0xCBF43926u);    // CRC32-Referenzwert
  puts("calib_store_test ok");
}
