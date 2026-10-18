#!/bin/bash
# Boot a built SM7150 UEFI FD inside qemu-system-aarch64 -M virt and collect the
# firmware's output.
#
# usage: run-fd.sh <SM7150_UEFI.fd> [outdir] [ram_mb] [wait_seconds] [fd_base]
#
# environment:
#   SERIAL   QEMU's -serial value (default "null").  The sunfishqemu profile is
#            built with --pl011, so
#              SERIAL=file:serial.log ./run-fd.sh <fd>
#            yields a plain-text DEBUG log (PrePi, DXE and the exception dump).
#            Without it the device profile's framebuffer text is decoded
#            instead.
#   TRACE    1 = also log every exception to $OUT/int.log (`-d int`).  Very
#            verbose: an IRQ storm can produce gigabytes in seconds, so it is
#            off by default.
#   GUESTLOG 0 = do not write $OUT/qemu.log (`-d guest_errors,unimp`).
#   VERBOSE  1 = print the whole log, not just the tail.
#   FONT5X12 path to the 5x12 font used by the framebuffer decoder.
set -eu
FD=${1:?usage: run-fd.sh <SM7150_UEFI.fd> [outdir] [ram_mb] [wait_s] [fd_base]}
OUT=${2:-./qemu-out}
RAM=${3:-3072}
WAIT=${4:-30}
FDBASE=${5:-0xCE000000}
SERIAL=${SERIAL:-null}
TRACE=${TRACE:-0}
GUESTLOG=${GUESTLOG:-1}
VERBOSE=${VERBOSE:-0}
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

QEMU_EXTRA=()
if [ "$TRACE" = "1" ]; then
  QEMU_EXTRA+=(-d int -D "$OUT/int.log")
elif [ "$GUESTLOG" = "1" ]; then
  QEMU_EXTRA+=(-d guest_errors,unimp -D "$OUT/qemu.log")
fi

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
  -no-reboot -no-shutdown -serial "$SERIAL" \
  "${QEMU_EXTRA[@]}" &
QPID=$!
python3 /tmp/qemu-fd-client-$$.py "$MON" "$OUT" "$WAIT" || true
wait $QPID 2>/dev/null || true
rm -f "$MON" /tmp/qemu-fd-client-$$.py

grep -a -E 'PC=' "$OUT/registers.txt" | head -1 || true

SERIAL_HAS_OUTPUT=0
case "$SERIAL" in
  file:*) SFILE=${SERIAL#file:}
          if [ -s "$SFILE" ]; then
            tr -d '\r' < "$SFILE" | sed 's/\x1b\[[0-9;]*m//g' > "$OUT/serial.txt"
            SERIAL_HAS_OUTPUT=1
            echo "serial log: $OUT/serial.txt ($(wc -l < "$OUT/serial.txt") lines)"
            echo "--- last 15 lines ---"
            tail -n 15 "$OUT/serial.txt"
            echo "--- last drivers loaded ---"
            grep -o 'Loading driver at 0x[0-9A-F]* EntryPoint=0x[0-9A-F]* [A-Za-z0-9_.-]*' \
              "$OUT/serial.txt" | tail -5 || true
            echo "--- exception / assert lines ---"
            grep -n -i 'Synchronous Exception\|ASSERT\|Data Abort\|^ERROR' "$OUT/serial.txt" \
              | tail -8 || true
            if [ "$VERBOSE" = "1" ]; then
              echo "--- full log ---"
              cat "$OUT/serial.txt"
            fi
          else
            echo "serial log: empty (no output reached the PL011)"
          fi ;;
esac

[ -f "$OUT/qemu.log" ] && [ -s "$OUT/qemu.log" ] && \
  echo "qemu device log: $OUT/qemu.log ($(wc -l < "$OUT/qemu.log") lines)"

[ -f "$OUT/fb.bin" ] || { echo "no framebuffer dump produced"; exit 0; }

if [ "$SERIAL_HAS_OUTPUT" = "1" ]; then
  echo "framebuffer dump kept at $OUT/fb.bin (console is on the PL011)"
  exit 0
fi

FONT="${FONT5X12:-$(cd "$HERE/../.." && pwd)/Silicon/Qualcomm/QcomPkg/Include/Resources/font5x12.h}"
python3 "$HERE/decode-fb.py" "$OUT/fb.bin" "$FONT" > "$OUT/fb.txt"
echo "decoded framebuffer log: $OUT/fb.txt ($(grep -c . "$OUT/fb.txt") non-empty lines)"
if [ "$VERBOSE" = "1" ]; then cat "$OUT/fb.txt"; else tail -n 40 "$OUT/fb.txt"; fi
