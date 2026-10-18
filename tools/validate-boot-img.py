#!/usr/bin/env python3
"""Validate boot-sunfish.img against the Pixel 4a ABL rules from the project notes.

A DTB stores its magic as big-endian 0xd00dfeed, i.e. the bytes d0 0d fe ed
appear literally in that order - do NOT byte-reverse before comparing.
"""
import struct, sys, zlib

P = sys.argv[1] if len(sys.argv) > 1 else \
    "/mnt/btrfs-1t/@home/batya/uefi-sunfish/edk2-msm/boot-sunfish.img"
d = open(P, "rb").read()
print(f"image: {P}\nsize : {len(d)} bytes ({len(d)/1024/1024:.2f} MiB)\n")

ok = True
def chk(label, got, want, note=""):
    global ok
    good = got == want
    ok &= good
    print(f"  [{'OK  ' if good else 'FAIL'}] {label:20s} got {got!r:16s} want {want!r} {note}")

print("=== boot_img_header ===")
chk("magic", d[:8], b"ANDROID!")
if d[:8] != b"ANDROID!":
    sys.exit("not an android boot image")

(k_size, k_addr, r_size, r_addr, s_size, s_addr, tags, page, ver) = \
    struct.unpack_from("<9I", d, 8)
chk("header_version", ver,   0,      "(ABL отвергает v2)")
chk("page_size",      page,  4096)
chk("kernel_addr",    k_addr, 0x8000)
chk("ramdisk_addr",   r_addr, 0x1000000)
chk("second_addr",    s_addr, 0x00f00000, "(mkbootimg обнуляет)")
chk("tags_addr",      tags,  0x100)
chk("ramdisk_size>0", r_size > 0, True)
chk("second_size==0", s_size, 0)

print(f"\n  kernel_size  = {k_size} ({k_size/1024/1024:.2f} MiB)")
print(f"  ramdisk_size = {r_size} ({r_size/1024/1024:.2f} MiB)")

print("\n=== payload ===")
pay = d[page:page + k_size]
if pay[:2] != b"\x1f\x8b":
    chk("payload is gzip", pay[:4].hex(), "1f8b0800"); sys.exit(1)
print("  [OK  ] payload is a gzip stream")

o = zlib.decompressobj(16 + zlib.MAX_WBITS)
raw = o.decompress(pay)
tail = o.unused_data
print(f"  gzip -> {len(raw)} bytes; {len(tail)} bytes appended after the stream")

print("\n=== appended DTB ===")
chk("FDT magic", tail[:4], bytes.fromhex("d00dfeed"), "(d0 0d fe ed, as-is)")
if len(tail) >= 24:
    totalsz, off_struct, off_str = struct.unpack_from(">3I", tail, 4)
    dtb_ver = struct.unpack_from(">I", tail, 20)[0]
    chk("dtb totalsize", totalsz, len(tail))
    chk("dtb version",   dtb_ver, 17)

print("\n=== RESULT ===")
print("  PASS - образ соответствует правилам ABL Pixel 4a" if ok else "  FAIL")
sys.exit(0 if ok else 1)
