# SkiSense BLE-Protokoll

Abgeleitet aus `BLE/Bluetooth/Bluetooth.ino`. Das Board ist BLE-Peripheral, die App verbindet sich als Central.

## Gerät

| Eigenschaft | Wert |
|---|---|
| GAP-Gerätename | `SkiSense-RP2040` |
| Advertising-Name (Local Name) | `SkiSense-TEST` |
| Advertised Service | `12345678-1234-5678-1234-56789abcdef0` |

Zum Finden des Geräts nach Local Name oder Service-UUID scannen. Nach dem Trennen advertised das Board automatisch wieder.

## Service und Characteristics

Service: `12345678-1234-5678-1234-56789abcdef0`

| UUID | Name | Eigenschaften | Länge |
|---|---|---|---|
| `12345678-1234-5678-1234-56789abcdef1` | Orientation | Read, Notify | 16 B |
| `12345678-1234-5678-1234-56789abcdef3` | Control | Read, Write, WriteWithoutResponse, Notify | 1 B |
| `12345678-1234-5678-1234-56789abcdef4` | Features | Read, Notify | 16 B |
| `12345678-1234-5678-1234-56789abcdef5` | Press4 | Read, Notify | 12 B |

Es gibt keine Characteristic `…def2` (früher belegt, entfernt).

## Allgemeines Format

- **Byte-Reihenfolge:** Little Endian (nativ auf dem RP2040, die Werte werden per `memcpy` kopiert).
- **Floats:** IEEE-754 `float32`.
- **`ts`:** `uint32`, `millis()` des Boards in ms seit dem Start. Läuft nach ca. 49,7 Tagen über. Der Zeitstempel kommt beim Packen, nicht beim Messen, und ist **nicht** mit der Uhr der App synchronisiert.
- Pakete haben feste Länge und keinen Header, keine Sequenznummer, keine Prüfsumme. Paketverlust ist nur über Lücken im `ts` erkennbar.
- Alle 16-B-Pakete passen in den Default-ATT-MTU (23 B, 20 B Nutzdaten).

### Orientation (`…def1`, 16 B)

| Offset | Typ | Feld | Einheit |
|---|---|---|---|
| 0 | float32 | `roll` | ° (Rist-IMU) |
| 4 | float32 | `pitch` | ° (Schienbein-IMU, mit `0.9·alt + 0.1·neu` geglättet) |
| 8 | float32 | `yaw` | ° (Rist-IMU, 0 … 360) |
| 12 | uint32 | `ts` | ms |

Roll/Pitch kommen aus einem Komplementärfilter (α = 0,98) aus Gyro und Beschleunigung. Yaw wird nur aus dem Gyro integriert, da kein Magnetometer genutzt wird, und **driftet** daher.

### Features (`…def4`, 16 B)

| Offset | Typ | Feld | Einheit |
|---|---|---|---|
| 0 | float32 | `edge_deg` | ° (Kantenwinkel = Rist-Roll) |
| 4 | float32 | `yaw_rate` | °/s (gewichtetes Mittel beider IMUs, ruhigerer Sensor zählt mehr) |
| 8 | float32 | `a_lat` | m/s² (Querbeschleunigung, ohne Schwerkraft) |
| 12 | uint32 | `ts` | ms |

### Press4 (`…def5`, 12 B)

| Offset | Typ | Feld | Einheit |
|---|---|---|---|
| 0 | uint16 | `fsr[0]` | Roh-ADC (A0) |
| 2 | uint16 | `fsr[1]` | Roh-ADC (A1) |
| 4 | uint16 | `fsr[2]` | Roh-ADC (derzeit Kopie von `fsr[0]`) |
| 6 | uint16 | `fsr[3]` | Roh-ADC (derzeit Kopie von `fsr[1]`) |
| 8 | uint32 | `ts` | ms |

`analogRead` liefert im Arduino-Default 10 Bit, also 0 … 1023. Es findet keine Umrechnung in Kraft statt. A2/A3 sind im Code auskommentiert.

### Control (`…def3`, 1 B)

| Wert | Bedeutung |
|---|---|
| `0x00` | Stop |
| `0x01` | Go (Streaming starten) |

Die Firmware schreibt nach jedem gültigen Befehl den aktuellen Zustand zurück (Notify). Bei unbekannten Werten bleibt der Zustand, er wird ebenfalls zurückgespiegelt. Nach dem Start und nach jedem Verbindungsende ist der Zustand `0x00`. Die App muss pro Verbindung `0x01` schreiben, sonst kommen keine Daten.

## Abtastrate

| Stufe | Rate |
|---|---|
| IMU lesen und filtern | 200 Hz (alle 5 ms) |
| BLE-Notify (Orientation, Features, Press4) | 100 Hz (alle 10 ms, jede zweite Messung) |

Die drei Characteristics werden im selben Zyklus nacheinander geschrieben. Ob jedes Notify ankommt, hängt vom Connection Interval des Centrals ab. Bei einem Intervall über 10 ms gehen Werte verloren oder werden ersetzt. Beim IMU-Lesen bleibt der letzte Wert stehen, wenn der Sensor noch keine neuen Daten hat.

## Sensor-Einstellungen

| Größe | Einheit / Bereich |
|---|---|
| Beschleunigung (intern) | g, Messbereich ±2 g (Library-Default, im Sketch nicht gesetzt) |
| Gyro (intern) | °/s, Messbereich ±250 °/s (Library-Default, im Sketch nicht gesetzt) |
| Tiefpass vor der Fusion | Biquad 2. Ordnung, Acc 40 Hz, Gyro 60 Hz (bei 200 Hz) |

Montage-Offsets und Ruhe-Offsets der IMUs sind aktuell 0.

## Ablauf

1. Scannen nach `SkiSense-TEST` bzw. Service-UUID, verbinden.
2. Notify auf Orientation, Features, Press4 (und optional Control) aktivieren.
3. `0x01` in Control schreiben, danach kommen die Daten mit 100 Hz.
4. `0x00` schreiben oder die Verbindung trennen, um zu stoppen.
