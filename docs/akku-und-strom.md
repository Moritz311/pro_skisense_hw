# Akkustand und Stromverbrauch

## Akkustand: Befund zur Hardware

Der Nano RP2040 Connect hat **keinen Messpunkt für die Akkuspannung** und keine Ladeschaltung. Ein freier Analogeingang für einen Spannungsteiler fehlt auch:

| Pins | Belegung |
|---|---|
| A0–A3 (einzige ADC-Pins des RP2040) | FSR 1–4 |
| A4/A5 | I²C (IMUs) |
| A6/A7 | hängen am NINA-Modul, nur über WiFiNINA lesbar, im BLE-Betrieb nicht nutzbar |

**Vorgeschlagene Lösung:** ein Fuel-Gauge **MAX17048** (z. B. Adafruit 5580 oder SparkFun TOL-20680) am vorhandenen I²C-Bus, Adresse `0x36` (kollidiert nicht mit den IMUs `0x68`/`0x69`). Er misst die LiPo-Zelle direkt und liefert den Ladestand in Prozent, ohne ADC-Pin und ohne Spannungsteiler, der dauerhaft Strom zieht.

Verdrahtung: `VBAT`/`GND` des Gauges an die Akkuzelle, `SDA`/`SCL`/`GND` an A4/A5/GND des Nano. Die Firmware erkennt den Chip beim Start automatisch:

- Gauge gefunden → Standard-BLE-Battery-Service `0x180F` mit Battery Level `0x2A19` (1 Byte, 0–100 %, Read + Notify). Aktualisiert alle 30 s, Notify nur bei Änderung.
- Kein Gauge → der Service fehlt, die App zeigt „—“. Seriell steht `Kein Fuel-Gauge, Battery Service aus.`

## Stromsparen in der Firmware

- IMUs schlafen, solange nicht gestreamt wird (`imu.sleep(true)`), und werden bei GO geweckt.
- Ohne Streaming wartet die Loop 10 ms zwischen den BLE-Polls statt im Kreis zu laufen.
- Advertising-Intervall 417,5 ms statt 100 ms. Die App findet das Gerät dadurch etwas später (im Mittel < 0,5 s).

Nicht per Firmware lösbar: Das NINA-Modul (ESP32, BLE-Radio) und die FSR-Spannungsteiler ziehen dauerhaft Strom. Ein echter Aus-Zustand braucht einen Schalter.

## Stromverbrauch messen

Benötigt: Multimeter mit mA-Bereich (besser: USB-Leistungsmesser oder Nordic PPK2), LiPo bzw. Labornetzteil 3,7 V.

1. Firmware flashen, danach **USB abziehen** (USB versorgt sonst mit und verfälscht die Messung).
2. Multimeter im mA-Bereich in Reihe zwischen Akku-Plus und `VIN` des Nano schalten.
3. Jeweils ca. 30 s warten und den Mittelwert ablesen:

| Zustand | So herstellen | Wert |
|---|---|---|
| A: Advertising | Gerät einschalten, App nicht verbinden | ___ mA |
| B: Verbunden, Leerlauf | In der App verbinden, nicht GO drücken | ___ mA |
| C: Streaming | GO drücken | ___ mA |

4. Zum Vergleich den Stand vor diesem Branch (`main`) flashen und A–C wiederholen. Erwartet: A und B einige mA niedriger (IMUs allein ~6 mA), C unverändert. Den Großteil zieht weiterhin das NINA-Modul.
5. Laufzeit abschätzen: `Akkukapazität (mAh) / Strom (mA) = Stunden`, z. B. 500 mAh / 40 mA ≈ 12 h.

Ein Multimeter zeigt bei Advertising schwankende Werte (Funkspitzen). Für saubere Mittelwerte einen Leistungsmesser mit Mittelung oder einen PPK2 verwenden.
