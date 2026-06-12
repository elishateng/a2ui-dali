#!/bin/bash
# capture_gallery.sh — capture the A2UI gallery (answer-key reference) via headless Chrome.
#
# The A2UI composer gallery (https://a2ui-composer.ag-ui.com/gallery) renders the same
# example set the reference renderer ships under res/gallery, using its own web renderer. We use it
# as a LAYOUT / CONTENT / STRUCTURE cross-check for a2ui-dali renders.
#
# NOTE on styling: the gallery's web styling is NOT identical to the reference renderer's OneUI.
# Per the parity brief the styling truth is the reference renderer's OneUI source spec; the gallery
# is only a layout/content reference. (Decision: OneUI styling wins on conflicts.)
#
# Usage: tools/capture_gallery.sh [out.png] [width] [height]
set -u
OUT="${1:-docs/autoplan/gallery_ref/gallery_full.png}"
W="${2:-1400}"
H="${3:-6000}"
URL="${A2UI_GALLERY_URL:-https://a2ui-composer.ag-ui.com/gallery}"
CHROME="$(which google-chrome google-chrome-stable chromium 2>/dev/null | head -1)"

[ -z "$CHROME" ] && { echo "[gallery] no chrome found" >&2; exit 2; }
mkdir -p "$(dirname "$OUT")"
"$CHROME" --headless=new --disable-gpu --no-sandbox --hide-scrollbars \
  --window-size="${W},${H}" --virtual-time-budget=15000 \
  --screenshot="$OUT" "$URL" 2>/dev/null
[ -s "$OUT" ] && echo "[gallery] OK → $OUT ($(stat -c%s "$OUT") bytes)" || { echo "[gallery] FAILED" >&2; exit 3; }
