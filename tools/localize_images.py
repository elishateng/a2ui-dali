#!/usr/bin/env python3
"""Download remote image URLs referenced by a corpus and rewrite them to local paths.

DALi's ImageView (in this build) does not fetch http(s) URLs, so remote images render as
grey placeholders. This pre-downloads every image-ish URL into res/sample-images/ and
writes a copy of each JSONL with the URLs replaced by local paths (relative to the
renderer's image dir, "res/"), so they load as local files.

Usage: localize_images.py <src_corpus_dir> <dst_corpus_dir>
"""
import json, glob, hashlib, os, re, sys, subprocess

IMG_KEY = re.compile(r'image|img|photo|avatar|poster|album|thumb|cover|picture|logo|art', re.I)
SRC_DIR, DST_DIR = sys.argv[1], sys.argv[2]
IMG_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'res', 'sample-images')
os.makedirs(IMG_DIR, exist_ok=True)
os.makedirs(DST_DIR, exist_ok=True)

urlmap = {}

def localize(url):
    if url in urlmap:
        return urlmap[url]
    fname = hashlib.md5(url.encode()).hexdigest()[:12] + '.jpg'
    dst = os.path.join(IMG_DIR, fname)
    if not os.path.exists(dst):
        r = subprocess.run(['curl', '-sL', '--max-time', '20', '-o', dst, url])
        ok = r.returncode == 0 and os.path.exists(dst) and os.path.getsize(dst) > 256
        if not ok:
            if os.path.exists(dst):
                os.remove(dst)
            urlmap[url] = None
            print(f'  MISS {url[:70]}')
            return None
        print(f'  GET  {os.path.getsize(dst):>7}B  {url[:60]}')
    rel = 'sample-images/' + fname        # relative to renderer image dir "res/"
    urlmap[url] = rel
    return rel

def walk(o, key=''):
    if isinstance(o, dict):
        return {k: walk(v, k) for k, v in o.items()}
    if isinstance(o, list):
        return [walk(x, key) for x in o]
    if isinstance(o, str) and '://' in o and IMG_KEY.search(key):
        loc = localize(o)
        return loc if loc else o          # keep original on download failure
    return o

n_files = n_urls = 0
for src in sorted(glob.glob(os.path.join(SRC_DIR, '*.jsonl'))):
    lines = []
    for line in open(src, encoding='utf-8'):
        line = line.strip()
        if not line:
            continue
        lines.append(json.dumps(walk(json.loads(line))))
    open(os.path.join(DST_DIR, os.path.basename(src)), 'w').write('\n'.join(lines) + '\n')
    n_files += 1

n_urls = sum(1 for v in urlmap.values() if v)
print(f'\nrewrote {n_files} files; localized {n_urls}/{len(urlmap)} image URLs into res/sample-images/')
