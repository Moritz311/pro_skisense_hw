# Graph Report - pro_skisense_hw  (2026-11-07)

## Corpus Check
- Corpus is ~176 words - fits in a single context window. You may not need a graph.

## Summary
- 8 nodes · 9 edges · 3 communities (0 shown, 3 thin omitted)
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- Community 1
- Community 2

## God Nodes (most connected - your core abstractions)
1. `mul()` - 3 edges
2. `eulerZYX_deg()` - 2 edges
3. `norm3()` - 2 edges

## Surprising Connections (you probably didn't know these)
- `mul()` --references--> `Vec3`  [EXTRACTED]
  BLE/Bluetooth/SkiSenseMath.h →   _Bridges community 1 → community 2_

## Import Cycles
- None detected.

## Communities (3 total, 3 thin omitted)

## Knowledge Gaps
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `mul()` connect `Community 1` to `Community 0`, `Community 2`?**
  _High betweenness centrality (0.254) - this node is a cross-community bridge._
- **Why does `eulerZYX_deg()` connect `Community 1` to `Community 0`?**
  _High betweenness centrality (0.087) - this node is a cross-community bridge._
- **Why does `norm3()` connect `Community 2` to `Community 0`?**
  _High betweenness centrality (0.087) - this node is a cross-community bridge._