# Graph Report - pro_skisense_hw  (2026-11-07)

## Corpus Check
- 6 files · ~2,357 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 4 file(s) not represented in the graph (top: .ino 4)

## Summary
- 67 nodes · 97 edges · 8 communities (6 shown, 2 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 4 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `354e42fa`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- SkiSenseMath.h
- Biquad
- BLE_Bluetooth_Bluetooth.cpp
- WIFI_Uebertragung.cpp
- WIFI_dauer.cpp
- Orient
- setup
- graphify-update.sh

## God Nodes (most connected - your core abstractions)
1. `Biquad` - 11 edges
2. `loop()` - 11 edges
3. `AxFilt` - 7 edges
4. `handleClient()` - 6 edges
5. `millis32()` - 5 edges
6. `Orient` - 5 edges
7. `setup()` - 5 edges
8. `eulerZYX_deg()` - 4 edges
9. `mul()` - 4 edges
10. `complementaryUpdate()` - 4 edges

## Surprising Connections (you probably didn't know these)
- `loop()` --calls--> `eulerZYX_deg()`  [INFERRED]
  graphify-mirror/BLE_Bluetooth_Bluetooth.cpp → BLE/Bluetooth/SkiSenseMath.h
- `setup()` --calls--> `eulerZYX_deg()`  [INFERRED]
  graphify-mirror/BLE_Bluetooth_Bluetooth.cpp → BLE/Bluetooth/SkiSenseMath.h
- `loop()` --calls--> `mul()`  [INFERRED]
  graphify-mirror/BLE_Bluetooth_Bluetooth.cpp → BLE/Bluetooth/SkiSenseMath.h
- `loop()` --calls--> `norm3()`  [INFERRED]
  graphify-mirror/BLE_Bluetooth_Bluetooth.cpp → BLE/Bluetooth/SkiSenseMath.h

## Import Cycles
- None detected.

## Communities (8 total, 2 thin omitted)

### Community 0 - "SkiSenseMath.h"
Cohesion: 0.32
Nodes (3): eulerZYX_deg(), mul(), norm3()

### Community 1 - "Biquad"
Cohesion: 0.15
Nodes (12): AxFilt, x, y, z, Biquad, a1, a2, b0 (+4 more)

### Community 2 - "BLE_Bluetooth_Bluetooth.cpp"
Cohesion: 0.30
Nodes (8): loop(), millis32(), packFeatures(), packOrientation(), packPress4_u16(), packPressure(), readOne(), setStreaming()

### Community 3 - "WIFI_Uebertragung.cpp"
Cohesion: 0.18
Nodes (5): loop(), Measurement, timestamp, values, sendData()

### Community 4 - "WIFI_dauer.cpp"
Cohesion: 0.46
Nodes (4): handleClient(), handlePing(), html(), loop()

### Community 5 - "Orient"
Cohesion: 0.33
Nodes (5): complementaryUpdate(), Orient, pitch, roll, yaw

## Knowledge Gaps
- **16 isolated node(s):** `b0`, `b1`, `b2`, `a1`, `a2` (+11 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 28 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Biquad` connect `Biquad` to `BLE_Bluetooth_Bluetooth.cpp`, `setup`?**
  _High betweenness centrality (0.154) - this node is a cross-community bridge._
- **Why does `AxFilt` connect `Biquad` to `BLE_Bluetooth_Bluetooth.cpp`, `setup`?**
  _High betweenness centrality (0.071) - this node is a cross-community bridge._
- **Why does `loop()` connect `BLE_Bluetooth_Bluetooth.cpp` to `SkiSenseMath.h`, `Biquad`, `Orient`?**
  _High betweenness centrality (0.068) - this node is a cross-community bridge._
- **Are the 3 inferred relationships involving `loop()` (e.g. with `eulerZYX_deg()` and `mul()`) actually correct?**
  _`loop()` has 3 INFERRED edges - model-reasoned connections that need verification._
- **What connects `b0`, `b1`, `b2` to the rest of the system?**
  _16 weakly-connected nodes found - possible documentation gaps or missing edges._