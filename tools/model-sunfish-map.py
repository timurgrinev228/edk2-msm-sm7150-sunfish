#!/usr/bin/env python3
"""Reduce the effective sunfish memory map and report what the result looks like.

Models what SunfishMemoryMapLib produces: the shared SM7150 table with the
HLOS 3 / HLOS 6b corrections, then the sunfish device and reserved regions, then
SunfishResolveOverlaps() (drop exact duplicates, carve reserved ranges out of the
allocatable ones).

Checks the three things that actually matter:

  * does the table still fit MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT (128)?
  * is any range still both allocatable and reserved?
  * are the remaining overlaps only device-vs-device or reserved-vs-reserved,
    neither of which reaches the allocator?

Usage:  model-sunfish-map.py
"""
import re, sys, os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHARED = f"{REPO}/Silicon/Qualcomm/sm7150/Library/PlatformMemoryMapLib/PlatformMemoryMapLib.c"
SUNFISH = f"{REPO}/Platform/Google/sm7150/Library/SunfishMemoryMapLib/SunfishMemoryMapLib.c"
MAXN = 128

HLOS3_BASE = 0x85F40000
SMEM_BASE  = 0x86000000
HLOS6B     = 0x9FFD0000


def shared_rows():
    t = open(SHARED, encoding="utf-8", errors="replace").read()
    t = t[t.find("{", t.find("gDeviceMemoryDescriptorEx")):t.rfind("};")]
    out = []
    for m in re.finditer(
            r'\{\s*"([^"]*)"\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,'
            r'\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,', t):
        out.append(dict(name=m.group(1), addr=int(m.group(2), 0), ln=int(m.group(3), 0),
                        hob=m.group(4), rt=m.group(5), mt=m.group(7)))
    return out


def sunfish_additions():
    txt = open(SUNFISH).read()
    defs = dict(re.findall(r'#define\s+(SUNFISH_\w+)\s+(0x[0-9A-Fa-f]+|\d+)ULL', txt))
    out = []
    for kind in ("Device", "Reserved"):
        pat = (rf'SunfishAdd{kind}Region\s*\(\s*mSunfishMemoryMap\s*,\s*&OutIndex\s*,\s*'
               rf'(SUNFISH_\w+)\s*,\s*(SUNFISH_\w+)\s*,\s*"([^"]+)"')
        for m in re.finditer(pat, txt):
            b, s = m.group(1), m.group(2)
            if b in defs and s in defs:
                out.append((m.group(3), int(defs[b], 0), int(defs[s], 0),
                            "AddDev" if kind == "Device" else "AddMem",
                            "MMAP_IO" if kind == "Device" else "MEM_RES",
                            "MmIO" if kind == "Device" else "Reserv"))
    return out


def build():
    base = shared_rows()
    for r in base:
        if r["addr"] == HLOS3_BASE:
            r["ln"] = SMEM_BASE - HLOS3_BASE
        if r["addr"] == HLOS6B:
            r["name"] = "HLOS 6b"
    adds = sunfish_additions()
    return ([dict(r) for r in base] +
            [dict(name=n, addr=a, ln=l, hob=h, rt=rt, mt=mt) for n, a, l, h, rt, mt in adds])


def resolve(m):
    """Mirror of SunfishResolveOverlaps()."""
    seen, out = set(), []
    for r in m:
        if r["ln"] == 0:
            break
        k = (r["addr"], r["ln"])
        if k in seen or len(out) >= MAXN - 1:
            continue
        seen.add(k)
        if r["hob"] != "AddMem" or r["rt"] != "SYS_MEM":
            out.append(r)
            continue
        reserved = [(x["addr"], x["addr"] + x["ln"]) for x in m
                    if x["hob"] == "AddMem" and x["rt"] == "MEM_RES"]
        pieces, cur = [], r["addr"]
        end = r["addr"] + r["ln"]
        for cs, ce in reserved:
            if ce <= cur or cs >= end:
                continue
            if cs > cur:
                pieces.append((cur, cs))
            cur = max(cur, ce)
        if cur < end:
            pieces.append((cur, end))
        for i, (a, e) in enumerate(p for p in pieces if p[1] > p[0]):
            c = dict(r); c["addr"] = a; c["ln"] = e - a
            c["name"] = r["name"] if i == 0 else f'{r["name"]}#{i}'
            out.append(c)
    return out


def conv_bytes(m):
    return sum(r["ln"] for r in m
               if r["hob"] == "AddMem" and r["rt"] == "SYS_MEM" and r["mt"] == "Conv")


def main():
    before = build()
    after = resolve(before)

    print(f"regions before resolve() : {len(before):3d}")
    print(f"regions after  resolve() : {len(after):3d} / {MAXN}  "
          f"{'FITS' if len(after) <= MAXN - 1 else 'DOES NOT FIT'}")
    print(f"allocatable before       : {conv_bytes(before)/1048576:8.1f} MiB")
    print(f"allocatable after        : {conv_bytes(after)/1048576:8.1f} MiB")

    print("\n=== ranges still both allocatable and reserved ===")
    alloc = [r for r in after if r["hob"] == "AddMem" and r["rt"] == "SYS_MEM"]
    res = [(x["addr"], x["addr"] + x["ln"]) for x in after
           if x["hob"] == "AddMem" and x["rt"] == "MEM_RES"]
    hits = 0
    for r in alloc:
        for a, b in res:
            if r["addr"] < b and a < r["addr"] + r["ln"]:
                hits += 1
                print(f"  {r['name']} 0x{r['addr']:X} vs reserved 0x{a:X}")
    print("  none" if not hits else f"  {hits} found")

    print("\n=== remaining overlaps (by class) ===")
    al = sorted([r for r in after if r["ln"] > 0], key=lambda r: r["addr"])
    kinds = {}
    for i in range(len(al) - 1):
        a, b = al[i], al[i + 1]
        if a["addr"] + a["ln"] > b["addr"]:
            kinds.setdefault((a["rt"], b["rt"]), 0)
            kinds[(a["rt"], b["rt"])] += 1
    if not kinds:
        print("  none")
    for (x, y), n in sorted(kinds.items()):
        note = ""
        if "SYS_MEM" in (x, y):
            note = "  <- reaches the allocator"
        print(f"  {x} vs {y}: {n}{note}")

    ok = (len(after) <= MAXN - 1) and hits == 0 and \
         not any("SYS_MEM" in k for k in kinds)
    print("\n=== RESULT:", "PASS" if ok else "FAIL", "===")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
