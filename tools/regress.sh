#!/bin/bash
# regress.sh — render all 29 gallery examples and diff against the golden baseline.
# A pure refactor must reproduce the golden renders pixel-for-pixel (renders are
# deterministic), so any non-zero mean-abs-diff flags a regression.
#
#   . setenv && tools/regress.sh
#
# The corpus inputs and golden PNGs are local dev data (git-ignored, like the other
# render dirs). To (re)create the golden baseline from a build you trust, render the
# corpus once into the golden dir:
#   for f in docs/autoplan/baseline/corpus_v09_img/*.jsonl; do
#     tools/capture.sh "$f" "docs/autoplan/golden/$(basename "$f" .jsonl).png" 480 1280 5
#   done
set -u
HERE="$(cd "$(dirname "$0")/.." && pwd)"
CORPUS="$HERE/docs/autoplan/baseline/corpus_v09_img"
GOLDEN="$HERE/docs/autoplan/golden"
OUT="/tmp/regress"
rm -rf "$OUT"; mkdir -p "$OUT"

for f in "$CORPUS"/*.jsonl; do
  name="$(basename "$f" .jsonl)"
  "$HERE/tools/capture.sh" "$f" "$OUT/$name.png" 480 1280 5 >/dev/null 2>&1
done

python3 - "$GOLDEN" "$OUT" <<'PY'
import sys, glob, os
from PIL import Image
import numpy as np
golden, out = sys.argv[1], sys.argv[2]
fails = 0; checked = 0
for g in sorted(glob.glob(os.path.join(golden, "*.png"))):
    name = os.path.basename(g)
    if name.startswith("_"): continue
    o = os.path.join(out, name)
    if not os.path.exists(o):
        print(f"  MISSING  {name}"); fails += 1; continue
    a = np.asarray(Image.open(g).convert("RGB"), dtype=float)
    b = np.asarray(Image.open(o).convert("RGB"), dtype=float)
    checked += 1
    if a.shape != b.shape:
        print(f"  SIZE     {name}  golden{a.shape} vs new{b.shape}"); fails += 1; continue
    d = abs(a - b).mean()
    if d > 0.05:   # tolerate sub-0.05 anti-alias noise; refactor should be ~0
        print(f"  DIFF {d:7.3f}  {name}"); fails += 1
print(f"\nregression: {checked-fails}/{checked} identical to golden" +
      (f"  —  {fails} CHANGED ⚠" if fails else "  —  all clean ✓"))
sys.exit(1 if fails else 0)
PY
