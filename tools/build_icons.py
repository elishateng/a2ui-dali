#!/usr/bin/env python3
"""build_icons.py — build a2ui-dali's icon set as tintable white-glyph PNGs.

Sources (in priority):
  1. A reference set of 58 Material SVGs (camelCase names) from REF_ICON_DIR.
  2. Material Symbols (outlined) fetched from GitHub raw for any corpus icon name not
     covered by (1) — these are the snake_case Material standard names the A2UI gallery uses.

Each icon is rasterized (cairosvg) then recolored to a WHITE glyph on transparent bg so
the renderer can tint it to any color via Ui::ImageView::SetImageColor (multiply).

Output: res/icons/<name>.png   (looked up by RenderIcon as mImageDir + "icons/" + name + ".png")
"""
import json, glob, os, io, urllib.request
import cairosvg
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
# Reference asset locations — set via environment (a local Material SVG set and a v0.8
# gallery corpus). They are only needed when regenerating res/icons; the built PNGs are
# committed under res/icons, so a normal build does not need these.
REF_ICON_DIR = os.environ.get("REF_ICON_DIR", "")
CORPUS = os.environ.get("REF_CORPUS_DIR", "")
OUT = os.path.join(HERE, "..", "res", "icons")
SIZE = 96
MAT_URL = ("https://raw.githubusercontent.com/google/material-design-icons/master/"
           "symbols/web/{n}/materialsymbolsoutlined/{n}_24px.svg")


def resolve_name(v, data):
    if isinstance(v, dict):
        if "literalString" in v:
            return v["literalString"]
        if "path" in v:
            key = v["path"].strip("/")
            return data.get(key) if key else None
    return v if isinstance(v, str) else None


def collect_corpus_names():
    names = set()
    for f in glob.glob(os.path.join(CORPUS, "*.json")):
        msgs = json.load(open(f))
        data = {}
        for m in msgs:
            if "dataModelUpdate" in m:
                for e in m["dataModelUpdate"].get("contents", []):
                    if "valueString" in e:
                        data[e["key"]] = e["valueString"]
        for m in msgs:
            for c in (m.get("surfaceUpdate", {}) or {}).get("components", []):
                cv = c.get("component", {})
                if isinstance(cv, dict) and "Icon" in cv:
                    n = resolve_name(cv["Icon"].get("name"), data)
                    if n and isinstance(n, str):
                        names.add(n)
    return names


def rasterize_white(svg_bytes, dst):
    png = cairosvg.svg2png(bytestring=svg_bytes, output_width=SIZE, output_height=SIZE)
    im = Image.open(io.BytesIO(png)).convert("RGBA")
    a = im.split()[3]                       # use only the glyph alpha
    white = Image.new("RGBA", (SIZE, SIZE), (255, 255, 255, 0))
    white.putalpha(a)                        # white glyph, transparent bg → tintable
    white.save(dst)


def main():
    os.makedirs(OUT, exist_ok=True)
    done, fail = set(), []

    # 1. reference parity set (camelCase)
    for svg in glob.glob(os.path.join(REF_ICON_DIR, "*.svg")) if REF_ICON_DIR else []:
        name = os.path.splitext(os.path.basename(svg))[0]
        try:
            rasterize_white(open(svg, "rb").read(), os.path.join(OUT, name + ".png"))
            done.add(name)
        except Exception as e:
            fail.append((name, str(e)[:60]))
    print(f"[1] reference parity icons: {len(done)}")

    # 2. corpus names (snake_case Material) not yet covered → fetch Material Symbols
    corpus = sorted(collect_corpus_names())
    print(f"[2] corpus icon names ({len(corpus)}): {corpus}")
    for name in corpus:
        dst = os.path.join(OUT, name + ".png")
        if name in done or os.path.exists(dst):
            continue
        try:
            req = urllib.request.Request(MAT_URL.format(n=name), headers={"User-Agent": "a2ui-icons"})
            svg = urllib.request.urlopen(req, timeout=20).read()
            rasterize_white(svg, dst)
            done.add(name)
            print("   fetched", name)
        except Exception as e:
            fail.append((name, str(e)[:60]))
            print("   MISS   ", name, str(e)[:60])

    print(f"\nDONE: {len(done)} icons → {os.path.relpath(OUT, HERE+'/..')}")
    if fail:
        print("FAILURES:", [f[0] for f in fail])


if __name__ == "__main__":
    main()
