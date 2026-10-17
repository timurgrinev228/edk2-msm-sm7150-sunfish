# SM7150 UEFI: QEMU test harness

The firmware built by this tree is *not* meant to run under QEMU (the SoC is not
emulated), but it can be executed inside `qemu-system-aarch64 -M virt` to
sanity-check the firmware image itself: SEC / PEI / DXE execute, the FV
decompresses, drivers get dispatched, and the device package does not break the
shared SoC/platform code.

How it works:

1. `qemu-system-aarch64 -M virt` resets the CPU at physical address `0x0` (ROM).
   We put a tiny stub there (`movz x5, #0xCE00, lsl #16 ; br x5`) and load the FD
   at its link address `0xCE000000` (`FD_BASE` from `configs/sm7150.conf`) with
   `-device loader`. That mimics what the ABL + `tools/BootShim/BootShim.bin` do
   on real hardware (BootShim copies the FD to `UEFI_BASE` and jumps to it,
   leaving `x0` = DTB pointer untouched).
2. This platform defaults to `USE_UART=0`, i.e. `FrameBufferSerialPortLib`, so
   **all EDK2 `DEBUG()` output is rendered as text into the framebuffer**
   (`PcdMipiFrameBufferAddress`, 5x12 font, scale factor 2). Inside QEMU that
   framebuffer is ordinary RAM, so it can be dumped through the QEMU monitor
   (`pmemsave`) and turned back into text by `decode-fb.py`.
3. The firmware will always die on SM7150-specific MMIO eventually, because that
   hardware does not exist in QEMU. What matters is *how far it gets* and *which
   module faults*.

## Usage

```bash
# normal build, for real hardware
./build.sh --device sunfish

# optional QEMU test profile (GIC / arch timer PCDs point at virt addresses)
./build.sh --device sunfishqemu --release DEBUG

tools/qemu-sunfish/run-fd.sh \
    workspace/Build/sunfish/RELEASE_CLANG38/FV/SM7150_UEFI.fd \
    /tmp/qemu-out 3072 30
```

Output files in the output directory:

| file            | content |
|:----------------|:--------|
| `fb.bin`        | raw 1080x2340 BGRA dump of the framebuffer (`0x9C000000`) |
| `fb.txt`        | decoded EDK2 log (what the firmware "printed") |
| `registers.txt` | `info registers` of CPU0 at dump time |
| `int.log`       | QEMU `-d int` guest exception log |

## Known results

RELEASE build, unmodified SM7150 PCDs:

```
Synchronous Exception at 0x00000000CA0A82C8
PC 0x0000CA0A82C8 (0x0000CA0A7000+0x000012C8) [ 0] ArmGicDxe.dll
X0 0x0000000017A00000 ...                  <- SM7150 GIC distributor base
Data abort: Translation fault, third level, FAR 0x0000000000000008
```

`sunfishqemu` profile (GIC at virt's `0x08000000`), DEBUG build:

```
SetUefiImageMemoryAttributes ...
ProtectUefiImageCommon ...
Loading driver DE371F7C-DEC4-4D21-ADF1-593ABCC15882
InstallProtocolInterface: 5B1B31A1-9562-11D2-8E3F-00A0C969723B C8831BC0
Loading driver at 0x000CA013000 EntryPoint=0x000CA014000 ArmGicDxe.efi
Synchronous Exception at 0x00000000CA014360
PC 0x0000CA014360 (0x0000CA013000+0x00001360) [ 0] ArmGicDxe.dll
ESR : EC 0x25 IL 0x1 ISS 0x00000006
Data abort: Translation fault, second level, FAR 0x0000000008000004
```

Both mean the same thing for the port: **the firmware image is fine**; only the
SoC-specific hardware access fails, which is expected off-device.
