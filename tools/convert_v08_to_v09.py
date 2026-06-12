#!/usr/bin/env python3
"""convert_v08_to_v09.py — convert the reference renderer v0.8 gallery corpus → a2ui-dali v0.9 JSONL.

the reference renderer's gallery test corpus (res/gallery/converted/*.json) is an array of v0.8
messages (surfaceUpdate / beginRendering / dataModelUpdate) with nested component
envelopes ({"component":{"Card":{...}}}) and literal/explicitList value wrappers.

a2ui-dali speaks v0.9 only: createSurface / updateComponents / updateDataModel, with
flat component envelopes ({"component":"Card", ...}) and plain values. This script does
the mechanical transform so the SAME logical UI can be rendered by a2ui-dali for
screenshot parity against the reference renderer.

Usage:
  tools/convert_v08_to_v09.py <input.json|dir> <output_dir>
Emits one <name>.jsonl per input file (one v0.9 message per line).
"""
import json, sys, os, glob

# v0.8 → v0.9 component TYPE renames
TYPE_RENAME = {
    "MultipleChoice": "ChoicePicker",   # v0.8 MultipleChoice → v0.9 ChoicePicker
}
# v0.8 → v0.9 property renames
RENAME = {
    "usageHint": "variant",     # Text / Image style hint
    "distribution": "justify",  # Row/Column main-axis distribution
    "alignment": "align",       # Row/Column cross-axis alignment
    "altText": "description",   # Image accessibility text
    "selections": "value",      # MultipleChoice selections → ChoicePicker value
    "minValue": "min",          # Slider
    "maxValue": "max",          # Slider
    "tabItems": "tabs",         # Tabs
    "entryPointChild": "trigger",  # Modal
    "contentChild": "content",     # Modal
}
# semantic gap → px (approx; true parity uses fixed Column/RowVariables.Gap — refined in M2)
GAP_PX = {"none": 0, "small": 8, "medium": 16, "large": 24}
# cross-axis values a2ui-dali doesn't support → nearest
ALIGN_FALLBACK = {"baseline": "start"}


def unwrap(v):
    """Unwrap a v0.8 value object to a plain v0.9 value (paths kept as bindings)."""
    if isinstance(v, dict):
        if len(v) == 1:
            (k, inner), = v.items()
            if k in ("literalString", "literalNumber", "literalBoolean"):
                return inner
            if k in ("literalArray", "explicitList"):
                return [unwrap(x) for x in inner]
            if k == "path":
                return {"path": inner}
        return {kk: unwrap(vv) for kk, vv in v.items()}
    if isinstance(v, list):
        return [unwrap(x) for x in v]
    return v


def transform_props(props):
    out = {}
    for k, v in (props or {}).items():
        if k == "gap" and isinstance(v, str):
            out["gap"] = GAP_PX.get(v, 8)
            continue
        if k == "children":
            out["children"] = unwrap(v)          # {explicitList:[...]}|{template}|[...] → array/obj
            continue
        if k == "action":
            av = unwrap(v)                        # v0.8 → v0.9 {event:{name,...}}
            if isinstance(av, str):
                out["action"] = {"event": {"name": av}}
            elif isinstance(av, dict) and "event" not in av and "functionCall" not in av:
                out["action"] = {"event": av}
            else:
                out["action"] = av
            continue
        nk = RENAME.get(k, k)
        uv = unwrap(v)
        if nk == "align" and isinstance(uv, str):
            uv = ALIGN_FALLBACK.get(uv, uv)
        out[nk] = uv
    return out


def flatten_component(c):
    comp = c.get("component")
    if isinstance(comp, dict) and len(comp) == 1:
        (type_name, props), = comp.items()
        type_name = TYPE_RENAME.get(type_name, type_name)
        newc = {"id": c["id"], "component": type_name}
        newc.update(transform_props(props))
        if "weight" in c:
            newc["weight"] = c["weight"]
        return newc
    return c  # already flat / unexpected — pass through


def contents_to_value(body):
    if "value" in body:
        return body["value"]
    contents = body.get("contents")
    if isinstance(contents, list):
        val = {}
        for e in contents:
            key = e.get("key")
            for vk in ("valueString", "valueNumber", "valueBoolean", "valueMap", "valueArray"):
                if vk in e:
                    val[key] = e[vk]
                    break
        return val
    return contents if contents is not None else {}


def convert_file(path):
    data = json.load(open(path, encoding="utf-8-sig"))  # tolerate UTF-8 BOM
    msgs = data if isinstance(data, list) else [data]
    lines, seen = [], set()

    # a2ui-dali renders the component whose id == "root" (the v0.9 convention). v0.8
    # beginRendering may name a different root (e.g. "rootlayout"); rename it to "root"
    # so the surface actually renders. (Nothing references the root by id, so this is safe.)
    root_id = next((m["beginRendering"].get("root") for m in msgs if "beginRendering" in m), None)

    def fix_root(c):
        if root_id and root_id != "root" and c.get("id") == root_id:
            c = dict(c); c["id"] = "root"
        return c

    def ensure_surface(sid):
        if sid not in seen:
            lines.append({"version": "v0.9",
                          "createSurface": {"surfaceId": sid, "catalogId": "basic"}})
            seen.add(sid)

    for m in msgs:
        if "surfaceUpdate" in m:
            b = m["surfaceUpdate"]; sid = b.get("surfaceId", "default"); ensure_surface(sid)
            comps = [flatten_component(fix_root(c)) for c in b.get("components", [])]
            lines.append({"version": "v0.9",
                          "updateComponents": {"surfaceId": sid, "components": comps}})
        elif "dataModelUpdate" in m:
            b = m["dataModelUpdate"]; sid = b.get("surfaceId", "default"); ensure_surface(sid)
            lines.append({"version": "v0.9",
                          "updateDataModel": {"surfaceId": sid,
                                              "path": b.get("path", "/"),
                                              "value": contents_to_value(b)}})
        elif "beginRendering" in m:
            ensure_surface(m["beginRendering"].get("surfaceId", "default"))  # root="root" convention
    return lines


def main():
    if len(sys.argv) != 3:
        print(__doc__); sys.exit(2)
    src, out_dir = sys.argv[1], sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)
    files = sorted(glob.glob(os.path.join(src, "*.json"))) if os.path.isdir(src) else [src]
    for f in files:
        lines = convert_file(f)
        name = os.path.splitext(os.path.basename(f))[0] + ".jsonl"
        with open(os.path.join(out_dir, name), "w") as fh:
            for ln in lines:
                fh.write(json.dumps(ln, ensure_ascii=False) + "\n")
        print(f"  {os.path.basename(f)} → {name} ({len(lines)} msgs)")
    print(f"converted {len(files)} file(s) → {out_dir}")


if __name__ == "__main__":
    main()
