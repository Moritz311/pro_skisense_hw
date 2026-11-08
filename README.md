# SkiSense Hardware

Firmware für den SkiSense-Sensor am Skischuh: zwei IMUs (Schienbein, Rist) und Drucksensoren (FSR). Die Daten gehen per BLE an die App [pro_skisense](https://github.com/Moritz311/pro_skisense).

| Sketch | Zweck |
|---|---|
| `BLE/Bluetooth/` | Hauptfirmware: IMU + FSR, Streaming per BLE an die App |
| `WIFI/WIFI_Uebertragung/` | WLAN-AP, misst FSR und liefert die Daten gesammelt über `GET /start`, `/data`, `/stop` |
| `WIFI/WIFI_dauer/` | WLAN-AP mit Live-Webseite (Server-Sent Events auf `/live`) |
| `WIFI/WIFI_test/` | Minimaler AP-Test |

## Hardware

- **Board:** Arduino Nano RP2040 Connect (FQBN `arduino:mbed_nano:nanorp2040connect`)
- **IMUs:** 2 × ICM-20948 (SparkFun 9DoF Breakout) an I²C (SDA = A4, SCL = A5, 400 kHz)
- **Drucksensoren:** FSR (Force Sensing Resistor) als Spannungsteiler an den Analogeingängen

### Verdrahtung

| Signal | Pin / Adresse |
|---|---|
| IMU Schienbein | I²C `0x68` (AD0 → GND) |
| IMU Rist | I²C `0x69` (AD0 → 3V3) |
| FSR 1 | A0 |
| FSR 2 | A1 |
| FSR 3, 4 | A2, A3 (vorgesehen, in der Firmware noch auskommentiert) |

## Bibliotheken

Mit `arduino-cli` installieren:

```sh
arduino-cli core install arduino:mbed_nano
arduino-cli lib install ArduinoBLE "SparkFun 9DoF IMU Breakout - ICM 20948 - Arduino Library" WiFiNINA
```

`WiFiNINA` brauchen nur die WIFI-Sketches.

## WLAN-Zugangsdaten

Die WIFI-Sketches lesen SSID und Passwort aus `secrets.h`. Die Datei ist per `.gitignore` ausgeschlossen. Vor dem Kompilieren die Vorlage in jeden Sketch-Ordner kopieren und ein eigenes Passwort eintragen (8–63 Zeichen):

```sh
for s in WIFI/WIFI_*/; do cp WIFI/secrets.example.h "$s/secrets.h"; done
```

> Das frühere Passwort `12345678` steht noch in der Git-Historie und gilt als verbrannt. Nicht wiederverwenden.

## Kompilieren & Flashen

```sh
arduino-cli compile -b arduino:mbed_nano:nanorp2040connect BLE/Bluetooth
arduino-cli board list                       # Port ermitteln
arduino-cli upload  -b arduino:mbed_nano:nanorp2040connect -p <port> BLE/Bluetooth
```

Falls das Board nicht auftaucht: Reset-Taste zweimal kurz drücken (Bootloader-Modus), dann erneut hochladen. Serieller Monitor: `arduino-cli monitor -p <port> -c baudrate=115200`.

## BLE-Protokoll

Gerätename `SkiSense-RP2040`, advertised als `SkiSense-TEST`.
Service `12345678-1234-5678-1234-56789abcdef0`. Alle Werte sind Little Endian, `ts` = `millis()` des Boards (uint32, ms).

| UUID-Endung | Name | Eigenschaften | Inhalt |
|---|---|---|---|
| `…def1` | Orientation | Read, Notify | 16 B: `float roll`, `float pitch`, `float yaw` (°), `u32 ts`. Roll/Yaw vom Rist, Pitch vom Schienbein (geglättet) |
| `…def3` | Control | Write, WriteWithoutResponse, Read, Notify | 1 B Befehl, siehe unten |
| `…def4` | Features | Read, Notify | 16 B: `float edge_deg` (Kantenwinkel), `float yaw_rate` (°/s), `float a_lat` (m/s²), `u32 ts` |
| `…def5` | Press4 | Read, Notify | 12 B: `u16 fsr[4]` (Roh-ADC, 10 Bit), `u32 ts`. FSR 3/4 sind derzeit Kopien von FSR 1/2 |

Control-Befehle:

| Wert | Bedeutung |
|---|---|
| `0x00` | Stop |
| `0x01` | Go (Streaming starten) |

Die Firmware spiegelt den aktuellen Zustand (`0x00`/`0x01`) per Notify auf Control zurück, auch bei unbekannten Befehlen. Intern werden die IMUs mit 200 Hz gelesen, die Notifies gehen mit 100 Hz raus. Beim Trennen der Verbindung stoppt das Streaming.
