#include <Arduino.h>
#include <Wire.h>
#include <ArduinoBLE.h>
#include <ICM_20948.h>   // SparkFun ICM-20948 Library
#include "SkiSenseMath.h"

// ===================== Geräte-/Service-Setup =====================
const char* DEVICE_NAME = "SkiSense-RP2040";
const char* LOCAL_NAME  = "SkiSense-TEST";

static const uint8_t CMD_STOP = 0x00;
static const uint8_t CMD_GO   = 0x01;

BLEService skiSenseService("12345678-1234-5678-1234-56789abcdef0");

BLECharacteristic      chOrientation("12345678-1234-5678-1234-56789abcdef1",
                                     BLERead | BLENotify, 16);
BLEByteCharacteristic  chControl    ("12345678-1234-5678-1234-56789abcdef3",
                                     BLEWrite | BLEWriteWithoutResponse | BLERead | BLENotify);
BLECharacteristic      chFeatures   ("12345678-1234-5678-1234-56789abcdef4",
                                     BLERead | BLENotify, 16);
BLECharacteristic      chPress4_u16("12345678-1234-5678-1234-56789abcdef5",
                                     BLERead | BLENotify, 12);

volatile bool streaming = false;

static inline uint32_t millis32() { return (uint32_t)millis(); }

// ===================== IMU Setup =====================
ICM_20948_I2C imu_shin;  // Schienbein
ICM_20948_I2C imu_rist;  // Rist

const bool AD0_IMU1 = false; // shin, 0x68
const bool AD0_IMU2 = true;  // rist, 0x69

static const float G_CONST       = 9.80665f;   // m/s^2

// ========== Montage-Offsets =====================
float SHIN_OFFS_ROLL  = 0;   // deg
float SHIN_OFFS_PITCH = 0;   // deg
float SHIN_OFFS_YAW   = 0;   // deg

float RIST_OFFS_ROLL  = 0;   // deg
float RIST_OFFS_PITCH = 0;   // deg
float RIST_OFFS_YAW   = 0;   // deg

Mat3 R_s2b_shin, R_s2b_rist; // Sensor->Boot Rotation

// ===================== IIR Tiefpass (Biquad) =====================
struct Biquad {
  float b0,b1,b2,a1,a2,z1=0,z2=0;
  void designLPF(float fs, float fc, float Q=0.7071f) {
    float w0=2*PI*fc/fs, c=cosf(w0), s=sinf(w0), alpha=s/(2*Q);
    float b0_= (1-c)*0.5f, b1_=(1-c), b2_=(1-c)*0.5f;
    float a0_= 1+alpha,    a1_= -2*c,  a2_= 1-alpha;
    b0=b0_/a0_; b1=b1_/a0_; b2=b2_/a0_; a1=a1_/a0_; a2=a2_/a0_;
    z1=z2=0;
  }
  inline float step(float x){
    float y=b0*x+z1; z1=b1*x - a1*y + z2; z2=b2*x - a2*y; return y;
  }
};
struct AxFilt {
  Biquad x,y,z;
  void design(float fs,float fc){x.designLPF(fs,fc);y.designLPF(fs,fc);z.designLPF(fs,fc);}
  void proc(float ix,float iy,float iz,float&ox,float&oy,float&oz){ox=x.step(ix);oy=y.step(iy);oz=z.step(iz);}
};
AxFilt accF_shin, gyrF_shin, accF_rist, gyrF_rist;

// ===================== Komplementärfilter je IMU =====================
struct Orient { float roll=0, pitch=0, yaw=0; }; // deg
static void complementaryUpdate(Orient &o, const Vec3& acc_g, const Vec3& gyr_dps, float dt, float alpha=0.98f){
  // Gyro integrieren
  o.roll  += gyr_dps.x * dt;
  o.pitch += gyr_dps.y * dt;
  o.yaw   += gyr_dps.z * dt; // driftet ohne Mag
  // Acc-Neigung
  float roll_acc  = atan2f(acc_g.y, acc_g.z) * 180.0f/PI;
  float pitch_acc = atan2f(-acc_g.x, sqrtf(acc_g.y*acc_g.y + acc_g.z*acc_g.z)) * 180.0f/PI;
  // Mischen
  o.roll  = alpha*o.roll  + (1-alpha)*roll_acc;
  o.pitch = alpha*o.pitch + (1-alpha)*pitch_acc;
  // yaw normalisieren
  if (o.yaw >= 360) o.yaw -= 360;
  if (o.yaw < 0)    o.yaw += 360;
}

// ===================== Konfiguration Raten/Filter =====================
const float FS_READ_HZ    = 200.0f;  // interner Filter/Lesetakt
const float FS_SEND_HZ    = 100.0f;  // BLE-Ausgaberate
const float ACC_CUTOFF_HZ = 40.0f;
const float GYR_CUTOFF_HZ = 60.0f;

float accOff_shin[3] = {0,0,0}, accOff_rist[3]={0,0,0};
float gyrOff_shin[3] = {0,0,0}, gyrOff_rist[3]={0,0,0};

// ===================== Laufzeit-Globals =====================
Orient ori_shin, ori_rist;
uint32_t readPeriodMs, sendPeriodMs, lastReadMs=0, sendAccumMs=0;

// ===================== Helpers =====================
static bool initIMU(ICM_20948_I2C &imu, bool ad0val) {
  if (imu.begin(Wire, ad0val) != ICM_20948_Stat_Ok) return false;
  // Optional: Full-Scale, DLPF etc. setzen (abhängig von Library-Version)
  return true;
}
// ===================== Pack-Helfer =====================
static void packOrientation(float roll, float pitch, float yaw, uint8_t* buf) {
  uint32_t ts = millis32();
  memcpy(buf + 0,  &roll,  4);
  memcpy(buf + 4,  &pitch, 4);
  memcpy(buf + 8,  &yaw,   4);
  memcpy(buf + 12, &ts,    4);
}
static void packPress4_u16(const uint16_t p[4], uint8_t* buf) {
  uint32_t ts = millis32();
  memcpy(buf + 0, &p[0], 2);
  memcpy(buf + 2, &p[1], 2);
  memcpy(buf + 4, &p[2], 2);
  memcpy(buf + 6, &p[3], 2);
  memcpy(buf + 8, &ts,   4);
}
static void packFeatures(float edge_deg, float yaw_rate_dps, float a_lat, uint8_t* buf) {
  uint32_t ts = millis32();
  memcpy(buf + 0,  &edge_deg,     4);
  memcpy(buf + 4,  &yaw_rate_dps, 4);
  memcpy(buf + 8,  &a_lat,        4);
  memcpy(buf + 12, &ts,           4);
}

// ===================== Control =====================
static void setStreaming(bool on) {
  streaming = on;
  lastReadMs = millis(); // sonst holt der Lesetakt die gesamte Pause nach
  chControl.writeValue(on ? CMD_GO : CMD_STOP);  // Zustand zurückspiegeln
  Serial.println(on ? "CMD: GO" : "CMD: STOP");
}

// ===================== IMU-Read =====================
static void readOne(ICM_20948_I2C &imu, float accOff[3], float gyrOff[3], float &ax, float &ay, float &az, float &gx, float &gy, float &gz)
{
  if (imu.dataReady()) {
    imu.getAGMT();
    ax = imu.accX()/1000.0f - accOff[0]; // g
    ay = imu.accY()/1000.0f - accOff[1];
    az = imu.accZ()/1000.0f - accOff[2];
    gx = imu.gyrX() - gyrOff[0]; // dps
    gy = imu.gyrY() - gyrOff[1];
    gz = imu.gyrZ() - gyrOff[2];
  }
}

// ===================== Setup =====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {} // ohne USB (Akkubetrieb) nicht ewig warten

  // I2C
  Wire.begin();
  Wire.setClock(400000);

  // IMUs
  if (!initIMU(imu_shin, AD0_IMU1) || !initIMU(imu_rist, AD0_IMU2)) {
    Serial.println("IMU init failed (Adressen/Verkabelung prüfen).");
    while (1) delay(1000);
  }
  Serial.println("IMUs bereit.");

  // Filter
  accF_shin.design(FS_READ_HZ, ACC_CUTOFF_HZ);
  gyrF_shin.design(FS_READ_HZ, GYR_CUTOFF_HZ);
  accF_rist.design(FS_READ_HZ, ACC_CUTOFF_HZ);
  gyrF_rist.design(FS_READ_HZ, GYR_CUTOFF_HZ);

  // Sensor->Boot Rotationen
  R_s2b_shin = eulerZYX_deg(SHIN_OFFS_YAW, SHIN_OFFS_PITCH, SHIN_OFFS_ROLL);
  R_s2b_rist = eulerZYX_deg(RIST_OFFS_YAW, RIST_OFFS_PITCH, RIST_OFFS_ROLL);

  // BLE
  if (!BLE.begin()) {
    Serial.println("BLE start failed!");
    while (1) delay(1000);
  }
  BLE.setDeviceName(DEVICE_NAME);
  BLE.setLocalName(LOCAL_NAME);
  BLE.setAdvertisedService(skiSenseService);

  skiSenseService.addCharacteristic(chOrientation);
  skiSenseService.addCharacteristic(chControl);
  skiSenseService.addCharacteristic(chFeatures);
  skiSenseService.addCharacteristic(chPress4_u16);
  BLE.addService(skiSenseService);

  // Initialwerte
  uint8_t z16[16]={0}; uint8_t z12[12]={0};
  chOrientation.writeValue(z16,16);
  chFeatures.writeValue(z16,16);
  chPress4_u16.writeValue(z12,12);
  setStreaming(false); // schreibt 0x00 in chControl

  BLE.advertise();
  Serial.println("BLE advertising als SkiSense-TEST ...");

  readPeriodMs = (uint32_t)(1000.0f/FS_READ_HZ + 0.5f);
  sendPeriodMs = (uint32_t)(1000.0f/FS_SEND_HZ + 0.5f);
  lastReadMs = millis();
}

// ===================== Loop =====================
void loop() {
  BLEDevice central = BLE.central();
  if (!central) { BLE.poll(); return; }

  Serial.print("Verbunden mit: ");
  Serial.println(central.address());

  sendAccumMs = 0;
  lastReadMs  = millis();

  while (central.connected()) {
    if (chControl.written()) {
      uint8_t cmd = 0xFF;
      chControl.readValue(&cmd, 1);
      if (cmd == CMD_GO) {
        if (!streaming) setStreaming(true);
      } else if (cmd == CMD_STOP) {
        if (streaming) setStreaming(false);
      } else {
        chControl.writeValue(streaming ? CMD_GO : CMD_STOP);
        Serial.print("Unbekannter CMD: 0x");
        Serial.println(cmd, HEX);
      }
    }

    // --- Datastrem GO ---
    if (streaming) {
      uint32_t now = millis();
      if (now - lastReadMs >= readPeriodMs) {
        lastReadMs += readPeriodMs;
        float dt = readPeriodMs/1000.0f;

        // ----- Read FSR -----
        uint16_t fsr[4];
        fsr[0] = analogRead(A0);
        fsr[1] = analogRead(A1);
        //fsr[2] = analogRead(A2);
        //fsr[3] = analogRead(A3);
        fsr[2] = fsr[0];
        fsr[3] = fsr[1];

        // ----- Read IMUs -----
        // static: readOne lässt die Werte unverändert, wenn keine neuen Daten da sind
        static float axS=0,ayS=0,azS=0,gxS=0,gyS=0,gzS=0; // shin
        static float axR=0,ayR=0,azR=0,gxR=0,gyR=0,gzR=0; // rist
        readOne(imu_shin, accOff_shin, gyrOff_shin, axS,ayS,azS,gxS,gyS,gzS);
        readOne(imu_rist, accOff_rist, gyrOff_rist, axR,ayR,azR,gxR,gyR,gzR);

        // ----- Filter IMU -----
        float faxS,fayS,fazS, fgxS,fgyS,fgzS;
        float faxR,fayR,fazR, fgxR,fgyR,fgzR;
        accF_shin.proc(axS,ayS,azS, faxS,fayS,fazS);
        gyrF_shin.proc(gxS,gyS,gzS, fgxS,fgyS,fgzS);
        accF_rist.proc(axR,ayR,azR, faxR,fayR,fazR);
        gyrF_rist.proc(gxR,gyR,gzR, fgxR,fgyR,fgzR);

        // ----- In BOOT-Frame drehen -----
        Vec3 accS_boot = mul(R_s2b_shin, Vec3{faxS,fayS,fazS});
        Vec3 gyrS_boot = mul(R_s2b_shin, Vec3{fgxS,fgyS,fgzS});
        Vec3 accR_boot = mul(R_s2b_rist, Vec3{faxR,fayR,fazR});
        Vec3 gyrR_boot = mul(R_s2b_rist, Vec3{fgxR,fgyR,fgzR});


        // ----- Orientierung je IMU -----
        complementaryUpdate(ori_shin, {accS_boot.x, accS_boot.y, accS_boot.z},
                                     {gyrS_boot.x, gyrS_boot.y, gyrS_boot.z}, dt, 0.98f);
        complementaryUpdate(ori_rist, {accR_boot.x, accR_boot.y, accR_boot.z},
                                     {gyrR_boot.x, gyrR_boot.y, gyrR_boot.z}, dt, 0.98f);

        // ----- Gewichte für Yaw-Rate (ruhigerer IMU zählt mehr) -----
        float egS = fabsf(norm3(accS_boot) - 1.0f); // ~| |a|-1g |
        float egR = fabsf(norm3(accR_boot) - 1.0f);
        float wS = 1.0f / (0.05f + egS);
        float wR = 1.0f / (0.05f + egR);
        float yaw_rate = (wS*gyrS_boot.z + wR*gyrR_boot.z) / (wS + wR); // °/s

        // ----- Edge Angle (Kantenwinkel) vom Rist-IMU -----
        float edge_deg = ori_rist.roll;

        // ----- Querbeschleunigung (m/s^2) -----
        Mat3 Rb2w = eulerZYX_deg(0, ori_rist.pitch, ori_rist.roll); // yaw egal für Grav
        Vec3 accR_world = mul(Rb2w, accR_boot);
        Vec3 accR_world_lin = {accR_world.x*G_CONST, accR_world.y*G_CONST, (accR_world.z-1.0f)*G_CONST};
        Mat3 Rw2b;
        for(int r=0;r<3;r++) for(int c=0;c<3;c++) Rw2b.m[r][c]=Rb2w.m[c][r];
        Vec3 acc_lin_boot = mul(Rw2b, accR_world_lin);
        float a_lat = acc_lin_boot.y;

        // ----- BLE senden @100 Hz -----
        sendAccumMs += readPeriodMs;
        if (sendAccumMs >= sendPeriodMs) {
          sendAccumMs -= sendPeriodMs;

          // Features
          uint8_t fbuf[16];
          packFeatures(edge_deg, yaw_rate, a_lat, fbuf);
          chFeatures.writeValue(fbuf, 16);

          // Orientation (Rist)
          uint8_t obuf[16];
          static float pitch_filt = 0; //to prevent trembling
          pitch_filt = 0.9f * pitch_filt + 0.1f * ori_shin.pitch;
          packOrientation(ori_rist.roll, pitch_filt, ori_rist.yaw, obuf);
          chOrientation.writeValue(obuf, 16);

          // Pressure Dummy
          uint8_t p4buf[12];
          packPress4_u16(fsr, p4buf);
          chPress4_u16.writeValue(p4buf, 12);
        }

        static uint32_t dbg=0; dbg += readPeriodMs;
        if (dbg >= 1000) {
          dbg=0;
          /*Serial.print(edge_deg);
          Serial.print(" ");
          Serial.print(yaw_rate);
          Serial.print(" ");
          Serial.print(a_lat);
          Serial.print(" ");
          Serial.print(fsr[0]);
          Serial.print(" ");
          Serial.print(fsr[1]);
          Serial.println();*/
          Serial.println("Senden läuft...");
        }
      }
    }

    BLE.poll();
  }

  Serial.println("Verbindung getrennt.");
  setStreaming(false);
  BLE.advertise();
}

