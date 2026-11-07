#!/bin/sh
# Graphify does not classify .ino; mirror sketches as .cpp so they enter the graph.
cd "$(dirname "$0")/.." || exit 1
rm -rf graphify-mirror && mkdir graphify-mirror
find . -name '*.ino' -not -path './graphify-mirror/*' -not -path './.git/*' | while read -r f; do
  out="graphify-mirror/$(echo "${f#./}" | tr '/' '_')"
  cp "$f" "${out%.ino}.cpp"
done
graphify update .
