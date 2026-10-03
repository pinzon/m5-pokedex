# /// script
# requires-python = ">=3.11"
# dependencies = ["requests", "pillow"]
# ///
"""Build data/dex.bin for the CoreInk Pokedex from PokeAPI (cached in tools/cache/).

Record layout (little endian, fixed RECORD_SIZE bytes) must match src/dex.h.
"""
import io
import json
import struct
import sys
import time
import unicodedata
from pathlib import Path

import requests
from PIL import Image, ImageChops, ImageFilter, ImageOps

ROOT = Path(__file__).resolve().parent.parent
CACHE = ROOT / "tools" / "cache"
PREVIEW = ROOT / "tools" / "preview"
OUT = ROOT / "data" / "dex.bin"
API = "https://pokeapi.co/api/v2"

COUNT = 386
SPRITE = 144
SPRITE_BYTES = SPRITE * SPRITE // 8
NAME_LEN, GENUS_LEN, FLAVOR_LEN = 12, 24, 240
RECORD_SIZE = 3072
HEADER = struct.Struct("<4sHH8x")  # magic, count, record size
RECORD = struct.Struct(f"<H{NAME_LEN}s{GENUS_LEN}sBBHH6B{FLAVOR_LEN}s{SPRITE_BYTES}s")
TYPES = ["normal", "fighting", "flying", "poison", "ground", "rock", "bug", "ghost", "steel",
         "fire", "water", "grass", "electric", "psychic", "ice", "dragon", "dark", "fairy"]
FLAVOR_PREF = ["emerald", "firered", "ruby", "sapphire", "leafgreen", "crystal", "gold", "red"]
PREVIEW_IDS = [1, 25, 150, 249, 384]
# Names whose API slug isn't a nice display name.
NAME_FIX = {"nidoran-f": "Nidoran F", "nidoran-m": "Nidoran M", "mr-mime": "Mr. Mime",
            "farfetchd": "Farfetch'd", "ho-oh": "Ho-Oh", "deoxys-normal": "Deoxys"}

session = requests.Session()


def fetch(url: str, path: Path, binary=False):
    if not path.exists():
        r = session.get(url, timeout=30)
        r.raise_for_status()
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(r.content)
        time.sleep(0.1)
    return path.read_bytes() if binary else json.loads(path.read_text())


def ascii_text(s: str) -> str:
    s = s.replace("­", "").replace("\f", " ").replace("\n", " ")
    s = unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode()
    s = " ".join(s.split())
    # Old games shout POKéMON / BERRIES; tone down the all-caps words.
    return " ".join(w.capitalize() if w.isupper() and len(w) > 1 else w for w in s.split())


def truncate_words(s: str, n: int) -> str:
    if len(s) <= n:
        return s
    return s[: n].rsplit(" ", 1)[0]


DARK, LIGHT = 80, 175  # gray < DARK: black, < LIGHT: 50% checker, else white


def to_sprite(png: bytes) -> Image.Image:
    """3-tone game-boy style: black outline + black/checker/white fill.

    Error-diffusion dithering turns upscaled pixel art into noise and washes out
    pale sprites, so tones are quantized and patterned at output resolution.
    """
    img = Image.open(io.BytesIO(png)).convert("RGBA")
    img = img.crop(img.getchannel("A").getbbox())
    mask = img.getchannel("A").point(lambda a: 255 if a > 127 else 0)
    edge = ImageChops.subtract(mask, mask.filter(ImageFilter.MinFilter(3)))
    bg = Image.new("RGBA", img.size, (255, 255, 255, 255))
    gray = ImageOps.autocontrast(Image.alpha_composite(bg, img).convert("L"), cutoff=1)

    scale = SPRITE / max(img.size)
    w, h = max(1, round(img.width * scale)), max(1, round(img.height * scale))
    gray, edge = gray.resize((w, h), Image.NEAREST), edge.resize((w, h), Image.NEAREST)

    out = Image.new("1", (SPRITE, SPRITE), 1)
    ox, oy = (SPRITE - w) // 2, (SPRITE - h) // 2
    gp, ep, op = gray.load(), edge.load(), out.load()
    for y in range(h):
        for x in range(w):
            v = gp[x, y]
            black = ep[x, y] or v < DARK or (v < LIGHT and (x + y) % 2 == 0)
            if black:
                op[ox + x, oy + y] = 0
    return out


def pack_bits(img: Image.Image) -> bytes:
    # PIL mode "1" tobytes is MSB-first rows with 1 = white; invert so 1 = black ink.
    return bytes(b ^ 0xFF for b in img.tobytes())


def build_record(i: int) -> tuple[bytes, Image.Image]:
    p = fetch(f"{API}/pokemon/{i}", CACHE / f"pokemon-{i}.json")
    s = fetch(f"{API}/pokemon-species/{i}", CACHE / f"species-{i}.json")
    png = fetch(p["sprites"]["front_default"], CACHE / f"sprite-{i}.png", binary=True)

    slug = s["name"]
    name = NAME_FIX.get(slug, slug.replace("-", " ").title())
    genus = next(g["genus"] for g in s["genera"] if g["language"]["name"] == "en")
    genus = ascii_text(genus).replace(" Pokemon", "")
    flavors = {f["version"]["name"]: f["flavor_text"] for f in s["flavor_text_entries"]
               if f["language"]["name"] == "en"}
    flavor = next((flavors[v] for v in FLAVOR_PREF if v in flavors), next(iter(flavors.values())))
    flavor = ascii_text(flavor)
    if len(flavor) > FLAVOR_LEN:
        print(f"  #{i}: flavor truncated {len(flavor)} -> {FLAVOR_LEN}")
        flavor = truncate_words(flavor, FLAVOR_LEN)

    types = sorted(p["types"], key=lambda t: t["slot"])
    t1 = TYPES.index(types[0]["type"]["name"])
    t2 = TYPES.index(types[1]["type"]["name"]) if len(types) > 1 else 0xFF
    stats = [st["base_stat"] for st in p["stats"]]
    assert len(stats) == 6 and all(v <= 255 for v in stats), (i, stats)
    assert len(name) <= NAME_LEN and len(genus) <= GENUS_LEN, (i, name, genus)

    sprite = to_sprite(png)
    rec = RECORD.pack(i, name.encode(), genus.encode(), t1, t2, p["height"], p["weight"],
                      *stats, flavor.encode(), pack_bits(sprite))
    return rec.ljust(RECORD_SIZE, b"\0"), sprite


def main():
    assert RECORD.size <= RECORD_SIZE, RECORD.size
    PREVIEW.mkdir(parents=True, exist_ok=True)
    out = bytearray(HEADER.pack(b"DEX1", COUNT, RECORD_SIZE))
    for i in range(1, COUNT + 1):
        rec, sprite = build_record(i)
        out += rec
        if i in PREVIEW_IDS:
            sprite.save(PREVIEW / f"{i:03d}.png")
        if i % 50 == 0:
            print(f"{i}/{COUNT}", file=sys.stderr)
    assert len(out) == HEADER.size + COUNT * RECORD_SIZE
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(out)
    print(f"wrote {OUT} ({len(out)} bytes, record {RECORD.size}/{RECORD_SIZE})")


if __name__ == "__main__":
    main()
