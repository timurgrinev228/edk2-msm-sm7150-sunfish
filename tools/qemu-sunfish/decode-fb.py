#!/usr/bin/env python3
"""Decode the EDK2 text that FrameBufferSerialPortLib rendered into the SM7150
framebuffer back into plain text.

usage: decode-fb.py <fb.bin> [font5x12.h] [width] [height] [bpp]
"""
import re
import sys

fb_path = sys.argv[1] if len(sys.argv) > 1 else "fb.bin"
font_path = sys.argv[2] if len(sys.argv) > 2 else "Silicon/Qualcomm/QcomPkg/Include/Resources/font5x12.h"
W = int(sys.argv[3]) if len(sys.argv) > 3 else 1080
H = int(sys.argv[4]) if len(sys.argv) > 4 else 2340
BPP = int(sys.argv[5]) if len(sys.argv) > 5 else 4

src = open(font_path, "r", errors="replace").read()
words = [int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{8})", src)]
glyph2char = {(words[i], words[i + 1]): chr(32 + i // 2)
              for i in range(0, len(words) - 1, 2)}

fb = open(fb_path, "rb").read()


def pixel(x, y):
    o = (y * W + x) * BPP
    return fb[o:o + BPP]


def decode(scale):
    cw, ch = (5 + 1) * scale, 12 * scale
    lines = []
    for r in range(min(H // ch, 400)):
        line = []
        for c in range(W // cw):
            w = [0, 0]
            for y in range(12):
                for x in range(5):
                    px = pixel(c * cw + x * scale, r * ch + y * scale)
                    if any(px[:3]):
                        w[y // 6] |= 1 << ((y % 6) * 5 + x)
            line.append(glyph2char.get((w[0], w[1]), "?" if (w[0] or w[1]) else " "))
        lines.append("".join(line).rstrip())
    return lines


best = None
for scale in (2, 1):
    lines = decode(scale)
    score = sum(1 for l in lines if l.strip())
    if best is None or score > best[0]:
        best = (score, lines)
for l in best[1]:
    if l.strip():
        print(l)
