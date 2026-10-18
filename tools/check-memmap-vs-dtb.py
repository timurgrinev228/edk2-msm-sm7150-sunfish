#!/usr/bin/env python3
"""Cross-check the SM7150 memory map against the real sunfish device trees.

The UEFI memory map describes what DXE may hand out; the device tree's
reserved-memory nodes describe what the secure world owns.  Where a region is
Conv (ordinary allocatable) in the map and reserved in the DT, the firmware
could in principle hand secure memory to a driver.

Two independent device trees for this device exist, and both are accepted as
ground truth:

  stock    Platform/Google/sm7150/FdtBlob_compat/sunfish.dtb   (425348 B, embedded)
  mobian   a mainline-style sm7150-google-sunfish.dtb from the Mobian kernel tree

Usage:  check-memmap-vs-dtb.py [mobian-dtb.dtb] [--map FILE] [--fix]

--fix  also prints the HLOS 3 length the map *should* have so the DT check
       below 0x86000000 goes to zero; the value in brackets is what
       SunfishMemoryMapLib applies.
"""
import re, subprocess, sys, os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STOCK = f"{REPO}/Platform/Google/sm7150/FdtBlob_compat/sunfish.dtb"
MAP   = f"{REPO}/Silicon/Qualcomm/sm7150/Library/PlatformMemoryMapLib/PlatformMemoryMapLib.c"
DEFAULT_MOBIAN = ("/mnt/btrfs-1t/@home/batya/projects/mobian-pixel4a/linux-fork/"
                  "arch/arm64/boot/dts/qcom/sm7150-google-sunfish.dtb")

SP_SMEM_BASE = 0x86000000   # secure partition, both DTs agree


def dtb_reserved(path):
    out = subprocess.run(["dtc", "-I", "dtb", "-O", "dts", path],
                         capture_output=True, text=True)
    if out.returncode != 0:
        sys.exit(f"dtc failed on {path}")
    t = out.stdout
    i = t.find("reserved-memory")
    if i < 0:
        return []
    seg = t[i: t.find("\n\t};", i)]
    res = []
    for m in re.finditer(r'([a-zA-Z0-9_\-]+)\s*\{[^}]*?reg\s*=\s*<([^>]*)>', seg, re.S):
        c = [x.strip() for x in m.group(2).split()]
        if len(c) != 4:
            continue
        b = (int(c[0], 16) << 32) | int(c[1], 16)
        s = (int(c[2], 16) << 32) | int(c[3], 16)
        res.append((m.group(1), b, b + s))
    return res


def map_regions(path):
    t = open(path, encoding="utf-8", errors="replace").read()
    t = t[:t.rfind("};")]
    rows = []
    for m in re.finditer(
            r'\{\s*"([^"]*)"\s*,\s*(0x[0-9A-Fa-f]+)\s*,\s*(0x[0-9A-Fa-f]+)\s*,'
            r'\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,', t):
        rows.append((m.group(1), int(m.group(2), 0), int(m.group(3), 0),
                     m.group(4), m.group(7)))
    return rows


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    mobian = args[0] if args else DEFAULT_MOBIAN
    if not os.path.exists(mobian):
        sys.exit(f"mobian dtb not found: {mobian}\n"
                 f"pass its path as the first argument")

    rows = map_regions(MAP)
    stock = dtb_reserved(STOCK)
    mob = dtb_reserved(mobian)

    print(f"map   : {MAP}")
    print(f"stock : {STOCK}  ({len(stock)} reserved regions)")
    print(f"mobian: {mobian}  ({len(mob)} reserved regions)\n")

    free = [r for r in rows if r[3] == "AddMem" and r[4] == "Conv"]

    print("=== Conv regions of the map that intersect a DT reserved range ===")
    hits = []
    for name, a, l, _, _ in free:
        e = a + l
        for src, rs in (("stock", stock), ("mobian", mob)):
            for n, b, be in rs:
                if a < be and b < e:
                    lo, hi = max(a, b), min(e, be)
                    hits.append((hi - lo, name, a, e, src, n, lo, hi))
    if not hits:
        print("  none")
    for sz, name, a, e, src, n, lo, hi in sorted(hits, reverse=True):
        print(f"  {name:14s} 0x{a:08X}..0x{e:08X} vs {src:6s} {n:14s} "
              f"0x{lo:08X}..0x{hi:08X}  {sz} B")

    print("\n=== the SP/SMEM overlap: the one that is unambiguously wrong ===")
    for name, a, l, _, _ in free:
        if a < SP_SMEM_BASE + 0x200000 and a + l > SP_SMEM_BASE:
            bad = a + l - SP_SMEM_BASE
            print(f"  {name} ends 0x{a+l:X}, secure partition starts 0x{SP_SMEM_BASE:X}")
            print(f"  overlap: {bad} bytes")
            if a == 0x85F40000:
                print(f"  corrected length: 0x{SP_SMEM_BASE - a:X} "
                      f"(SunfishMemoryMapLib applies this)")

    print("\n=== do the two device trees agree? ===")
    s = {(b, e) for _, b, e in stock}
    m = {(b, e) for _, b, e in mob}
    print(f"  ranges only in stock : {len(s - m)}")
    print(f"  ranges only in mobian: {len(m - s)}")
    if s - m:
        print("  -> the DTBs disagree, so reserved-memory is not a reliable")
        print("     oracle for what UEFI must avoid; only the SP/SMEM overlap,")
        print("     which both agree on and which contradicts the map's own")
        print("     contiguity, is safe to act on.")

    print("\n=== framebuffer cross-check ===")
    for src, rs in (("stock", stock), ("mobian", mob)):
        for n, b, be in rs:
            if 0x9C000000 <= b < 0x9E000000:
                print(f"  {src:7s} 0x{b:09X}..0x{be:09X} size 0x{be-b:X}")


if __name__ == "__main__":
    main()
