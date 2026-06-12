#!/bin/bash
# capture.sh — render an A2UI JSONL file with a2ui-dali and save a PNG screenshot.
#
# Usage:
#   tools/capture.sh <input.jsonl> <output.png> [width] [height] [render_wait_sec]
#
# Verified pipeline (Phase 0): Xvfb virtual framebuffer → a2ui-basic-renderer →
# xwd (X window dump) → ffmpeg (xwd→png). No real desktop / WM required.
# Requires: . setenv sourced (DESKTOP_PREFIX, LD_LIBRARY_PATH, dali2-*).
set -u

HERE="$(cd "$(dirname "$0")/.." && pwd)"
RENDERER="${A2UI_RENDERER:-$HERE/bin/a2ui-basic-renderer}"

IN="${1:?input jsonl required}"
OUT="${2:?output png required}"
W="${3:-720}"          # basic-renderer hardcodes 720x1080; match the screen to it
H="${4:-1080}"
WAIT="${5:-5}"         # seconds to let the surface render before capturing

if [ ! -x "$RENDERER" ]; then echo "[capture] renderer not found/executable: $RENDERER" >&2; exit 2; fi
if [ ! -f "$IN" ]; then echo "[capture] input not found: $IN" >&2; exit 2; fi
mkdir -p "$(dirname "$OUT")"

XWD="$(mktemp /tmp/a2ui_cap_XXXXXX.xwd)"

# env vars are inherited by the inner shell; DISPLAY is set by xvfb-run itself.
export RENDERER IN W H WAIT XWD
xvfb-run -a -s "-screen 0 ${W}x${H}x24" bash -c '
  export DALI_WINDOW_WIDTH="$W" DALI_WINDOW_HEIGHT="$H"
  "$RENDERER" "$IN" >/tmp/a2ui_render.log 2>&1 &
  app=$!
  sleep "$WAIT"
  xwd -root -display "$DISPLAY" -out "$XWD" 2>/tmp/a2ui_xwd.err
  kill "$app" 2>/dev/null; wait "$app" 2>/dev/null
'

if [ ! -s "$XWD" ]; then echo "[capture] xwd produced no data (see /tmp/a2ui_xwd.err)" >&2; exit 3; fi
ffmpeg -y -loglevel error -i "$XWD" "$OUT" || { echo "[capture] ffmpeg failed" >&2; exit 4; }
rm -f "$XWD"
[ -s "$OUT" ] && echo "[capture] OK → $OUT ($(stat -c%s "$OUT") bytes)" || { echo "[capture] PNG empty" >&2; exit 5; }
