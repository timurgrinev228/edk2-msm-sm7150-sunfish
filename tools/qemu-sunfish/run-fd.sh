#!/bin/bash
# Boot a built SM7150 UEFI FD inside qemu-system-aarch64 -M virt and dump the
# framebuffer (where this platform renders its DEBUG output).
#
# usage: run-fd.sh <SM7150_UEFI.fd> [outdir] [ram_mb] [wait_seconds] [fd_base]
set -eu
FD=${1:?usage: run-fd.sh <SM7150_UEFI.fd> [outdir] [ram_mb] [wait_s] [fd_base]}
OUT=${2:-./qemu-out}
RAM=${3:-3072}
WAIT=${4:-30}
FDBASE=${5:-0xCE000000}
HERE=$(cd "$(dirname "$0")" && pwd)

command -v qemu-system-aarch64 >/dev/null || { echo "qemu-system-aarch64 not found"; exit 1; }
[ -f "$FD" ] || { echo "no such FD: $FD"; exit 1; }
mkdir -p "$OUT"

# stub at the reset vector: movz x5, #(top 16 bits of FDBASE), lsl #16 ; br x5
python3 - "$FDBASE" > "$OUT/stub.bin" <<'PY'
import sys, struct
base = int(sys.argv[1], 0)
imm  = (base >> 16) & 0xFFFF
movz = 0xD2800000 | (0b01 << 21) | (imm << 5) | 5   # MOVZ x5, #imm, LSL #16
br   = 0xD61F0000 | (5 << 5)                        # BR x5
blob = struct.pack('<II', movz, br)
blob += b'\x00' * (4096 - len(blob))
sys.stdout.buffer.write(blob)
PY

MON="/tmp/qemu-fd-mon-$$.sock"
cat > /tmp/qemu-fd-client-$$.py <<'PY'
import json, os, socket, sys, time
sock, outdir, wait = sys.argv[1], sys.argv[2], float(sys.argv[3])
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
for _ in range(80):
    try:
        s.connect(sock); break
    except OSError:
        time.sleep(0.25)
else:
    print("could not connect to the QEMU monitor"); sys.exit(1)
s.settimeout(3.0)
def cmd(c, wait=2.0):
    s.sendall((c + "\n").encode()); time.sleep(wait)
    buf = b""
    while True:
        try:
            d = s.recv(1 << 20)
            if not d: break
            buf += d
        except socket.timeout:
            break
    return buf.replace(b"\x1b", b"").decode(errors="replace")
time.sleep(wait)
log = [cmd("stop")]
log.append(cmd('pmemsave 0x9C000000 0xA00000 "%s/fb.bin"' % outdir, 12.0))
log.append(cmd("info registers", 2.0))
log.append(cmd("quit"))
open("%s/registers.txt" % outdir, "w").write("\n".join(log))
PY

rm -f "$MON"
qemu-system-aarch64 \
  -M virt,gic-version=3 -cpu max -smp 8 -m "$RAM" -display none -net none \
  -bios "$OUT/stub.bin" \
  -device loader,file="$FD",addr="$FDBASE",force-raw=on \
  -monitor "unix:$MON,server,nowait" \
  -no-reboot -no-shutdown -serial null \
  -d int -D "$OUT/int.log" &
QPID=$!
python3 /tmp/qemu-fd-client-$$.py "$MON" "$OUT" "$WAIT" || true
wait $QPID 2>/dev/null || true
rm -f "$MON" /tmp/qemu-fd-client-$$.py

grep -a -E 'PC=' "$OUT/registers.txt" | head -1 || true
[ -f "$OUT/fb.bin" ] || { echo "no framebuffer dump produced"; exit 1; }

FONT="${FONT5X12:-$(cd "$HERE/../.." && pwd)/Silicon/Qualcomm/QcomPkg/Include/Resources/font5x12.h}"
python3 "$HERE/decode-fb.py" "$OUT/fb.bin" "$FONT" > "$OUT/fb.txt"
echo "decoded log: $OUT/fb.txt"
cat "$OUT/fb.txt"
