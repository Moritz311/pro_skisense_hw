#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---- Persistente Kalibrier-Offsets: Serialisierung + Validierung (hardwareunabhängig) ----
// Layout: magic | version | 12 Floats (acc/gyr shin, acc/gyr rist) | CRC32 über alles davor
static const uint32_t CALIB_MAGIC   = 0x534B4343; // "SKCC"
static const uint32_t CALIB_VERSION = 1;          // bei Layout-Änderung erhöhen -> alte Daten verworfen

struct CalibBlob {
  uint32_t magic, version;
  float accShin[3], gyrShin[3], accRist[3], gyrRist[3];
  uint32_t crc;
};

static inline uint32_t calibCrc32(const void* data, size_t len) {
  const uint8_t* p = (const uint8_t*)data;
  uint32_t c = 0xFFFFFFFFu;
  while (len--) {
    c ^= *p++;
    for (int i = 0; i < 8; i++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return ~c;
}

static inline void calibPack(CalibBlob& b, const float accS[3], const float gyrS[3],
                             const float accR[3], const float gyrR[3]) {
  memset(&b, 0, sizeof b);
  b.magic = CALIB_MAGIC; b.version = CALIB_VERSION;
  memcpy(b.accShin, accS, 12); memcpy(b.gyrShin, gyrS, 12);
  memcpy(b.accRist, accR, 12); memcpy(b.gyrRist, gyrR, 12);
  b.crc = calibCrc32(&b, offsetof(CalibBlob, crc));
}

// true nur bei passender Länge, Magic, Version und CRC; sonst bleibt out unverändert
static inline bool calibUnpack(const void* raw, size_t len, CalibBlob& out) {
  if (len != sizeof(CalibBlob)) return false;
  CalibBlob b; memcpy(&b, raw, sizeof b);
  if (b.magic != CALIB_MAGIC || b.version != CALIB_VERSION) return false;
  if (b.crc != calibCrc32(&b, offsetof(CalibBlob, crc))) return false;
  const float* f = b.accShin; // NaN/Inf trotz gültiger CRC ausschließen
  for (int i = 0; i < 12; i++) if (!(f[i] > -1e6f && f[i] < 1e6f)) return false;
  out = b;
  return true;
}
