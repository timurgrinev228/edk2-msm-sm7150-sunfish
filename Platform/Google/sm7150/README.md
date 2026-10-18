# Platform/Google/sm7150 - Google Pixel 4a (sunfish)

Package for the Google Pixel 4a 4G (codename `sunfish`, "GAHOU"/LAZER family,
Snapdragon 730G / SM7150 - the same SoC as Xiaomi davinci/surya and Sony a71).
The SoC-level support (`Platform/Qualcomm/sm7150`, `Silicon/Qualcomm/sm7150`)
is shared with those devices; everything here is the device-specific layer.

## Device package contents

| path | what |
|---|---|
| `sunfish.dsc` | platform DSC; selects the sunfish library variants below |
| `sunfish.fdf.inc` | per-device BSP DXEs (USB stack binaries) + ACPI table FFS |
| `platform.sh.inc` | boot image packaging hook (Google ABL shape, see below) |
| `Library/SunfishMemoryMapLib/` | sunfish DDR map. `#include`s the shared `Silicon/Qualcomm/sm7150` table and fixes the two defects that matter for this device (HLOS 3 used to overlap SMEM by 128 KiB; duplicate "HLOS 6" name), keeping upstream changes propagating |
| `Library/SunfishSmbiosInfoLib/` | SMBIOS strings (Google / Pixel 4a / sunfish) |
| `AcpiTables/sunfish/DSDT.aml` | ACPI DSDT used by the port (the shared SM7150 reference table; see `Dsdt.asl` below) |
| `AcpiTables/sunfish/Dsdt.asl` | rebuildable source of that table, iasl-verified (compiles to the same table with iasl 20250404; the URS0 `_HID` alias became a method because iasl >= 2023 rejects aliases for `_HID`) |
| `FdtBlob_compat/sunfish.dtb` | vendor DTB from the stock factory boot image (`qcom,sdmmagpie`) |
| `FdtBlob_compat/sunfish-ramdisk.img` | stock ramdisk, kept because the ABL requires a non-empty ramdisk section (contents are never parsed) |

## Boot image: what the Pixel 4a ABL accepts

Google's ABL rejects or hangs on the shapes Qualcomm's own ABL accepts.
The only image that boots is (`platform_build_bootimg` in `platform.sh.inc`):

```
header_version 0          page_size 4096
kernel   @ 0x00008000     ramdisk @ 0x01000000
second   @ 0x00f00000     tags    @ 0x00000100
kernel   = gzip(BootShim.bin + SM7150_UEFI.fd) ++ sunfish-kernel.dtb
ramdisk  = stock ramdisk blob (required, contents ignored)
```

Notes:

* `mkbootimg` zeroes `second_addr` whenever `second_size == 0`, so the hook
  patches the 4-byte field at offset 28 back to `0x00f00000` (outside the
  SHA1 `id` hash).
* deviation from the Xiaomi/`Silicon/platform.sh.inc` default (kernel_offset 0,
  ramdisk/second/tags 0, page_size 2048, header_version 1) is what makes the
  image accepted: wrong shape -> `Error verifying the received boot.img:
  Invalid Parameter`; a v2 header (even a copy of the stock one) without the
  matching payload hangs on the Google logo.
* ABL parses the payload as an arm64 Image: with the generic BootShim header
  (text_offset = image_size = flags = 0) it stops with `error boot prepare`.
  `platform_build_kernel` patches text_offset 0x80000, image_size and
  flags 0xa into the payload.
* UFSDxe resets the phone on sunfish and is left out (`DISABLE_UFS` in
  `sunfish.dsc`), so UEFI has no internal storage yet.

Build:

```sh
./build.sh --device sunfish        # -> ./boot-sunfish.img
```

`fastboot boot` (RAM only) does not work for unsigned images on the
Pixel 4a, so the image is flashed; keep the stock `boot.img` to go back:

```sh
fastboot flash boot_a boot-sunfish.img && fastboot --set-active=a && fastboot reboot
fastboot flash boot_a <stock boot.img> && fastboot reboot   # back to Android
```

## Library rewrite policy (davinci -> sunfish)

The original SoC libraries (`Silicon/Qualcomm/sm7150/Library/*`) were written
for the Xiaomi SM7150 devices and are shared by every port. The device layer
does **not** copy them; each variant instead:

* fixes only what is demonstrably device-specific (memory map: DTB-reserved
  regions are the reference),
* `#include`s the upstream table so future SoC-level changes are picked up,
* leaves shared code untouched so the Xiaomi/Samsung/A71 ports keep working.
