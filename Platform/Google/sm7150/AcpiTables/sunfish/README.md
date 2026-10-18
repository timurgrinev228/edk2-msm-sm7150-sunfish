# DSDT for Google Pixel 4a (sunfish)

`DSDT.aml` here is the table embedded in the firmware (see
`Platform/Google/sm7150/sunfish.fdf.inc`, FREEFORM `7E374E25-...`).

## Where it comes from

The original blob was byte-identical to the Xiaomi davinci/surya table
(md5 `65376a6d46bddd51e4b56a62759f17dd`): it is the shared Qualcomm SM7150
reference DSDT (`QCOMM` / `SDM8180`). Google ships no separate Pixel 4a DSDT,
and all the SoC-level ACPI in it (UFS, USB, PCIe, PMIC, PEP, CPPC, ...) applies
to sunfish unchanged.

`Dsdt.asl` is that binary disassembled with iasl, plus the device-specific
fixes below, so the table is a real source file instead of a black box.

## Rebuilding

```sh
./build.sh -d sunfish -A --skip-rootfs-gen     # -A / --acpi
```

`build.sh` copies `AcpiTables/<device>/*` (the `.asl` here and the common tables
from `Silicon/Qualcomm/sm7150/AcpiTables`) into the build directory, runs
`iasl -ve -p DSDT Dsdt.asl`, and copies `DSDT.aml` back. `SPLIT_DSDT` and
`USE_IASL` are enabled in `configs/devices/sunfish.conf` for that reason.

Manual rebuild (no full firmware build):

```sh
cd Platform/Google/sm7150/AcpiTables/sunfish
iasl -ve -p DSDT Dsdt.asl
```

## Changes vs the reference table

| what | reference (surya) | sunfish |
| --- | --- | --- |
| touch controller node `TSC1` | `\_SB.IC15` | `\_SB.IC10` (i2c7, `0xa84000`) |
| touch I2C address | `0x20` | `0x49` |
| touch I2C speed | 400 kHz | 1 MHz |
| touch interrupt | GPIO 122, edge, active low | GPIO 9, level, active low |
| touch reset | GPIO 54 | GPIO 8 |
| `URS0._HID` | `Alias (URSI, _HID)` | `Method (_HID) { Return (URSI ()) }` |

The last row is not sunfish-specific: iasl >= 2023 rejects
`Alias (<Method>, _HID)` with error 6141, and the method form is equivalent
ACPI. See the header of `Dsdt.asl`.

The touch HID stays `MSHW1003` (HID-over-I2C). Windows' `hidi2c` binds to it
and reads the HID report descriptor from the ST controller, so the panel
geometry (1080x2340) is *not* described in ACPI.

## Hardware inventory (mainline `sm7150-google-sunfish.dts`)

Source: `arch/arm64/boot/dts/qcom/sm7150-google-sunfish.dts` (and
`sm7150.dtsi`) in the mainline SM7150 tree.

| bus | device | address | notes |
| --- | --- | --- | --- |
| i2c2 @ 400 kHz | ST21NFC (`st,st21nfc`) | `0x08` | irq TLMM 37, enable TLMM 67 |
| i2c2 @ 400 kHz | Realtek RT5514 codec | `0x57` | |
| i2c7 @ 1 MHz | ST FTS5 touch (`st,stmfts5`) | `0x49` | irq TLMM 9, reset TLMM 8, mode-switch TLMM 72 |
| i2c9 @ 400 kHz | CS35L41 speaker amps | `0x40`, `0x41` | irq TLMM 64/65, reset PM6150L GPIO 8/11 |
| i2c9 @ 400 kHz | FSA4480 type-C mux | `0x43` | |
| i2c9 @ 400 kHz | DRV2624 haptics | `0x5a` | enable TLMM 11 |
| i2c9 @ 400 kHz | SLG51000 PMIC | `0x75` | CS on PM6150L GPIO 4/6 |
| uart3 | WCN3990 Bluetooth (`qcom,wcn3990-bt`) | | |
| uart8 | debug console | | `0xa88000` |
| UFS | `ufs_mem_hc` / `ufs_mem_phy` | | |
| DSI | panel `google,ams581vf01-sunfish` | | 1080x2340, framebuffer `0x9c000000` |

## Not described in this table (yet)

The reference table has no ACPI nodes at all for the i2c2/i2c9 peripherals
(`grep -c` for `ST21`, `RT5514`, `CS35L`, `DRV2624`, `FSA4480` is 0). Adding
them means inventing ACPI HIDs and `_DSM` GPIO maps that nothing in this tree
validates, so they are deliberately left out rather than guessed:

- NFC, audio codec + speaker amps, haptics, type-C mux, camera (`cci0/cci1`),
  WCN3990 Wi-Fi/BT.
- `Csrt`, `dbg2`, `IORT`, `SPCR` exist under
  `Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/` but are not listed in the FDF.
  `SPCR` points at a serial base that does not match the SM7150 GENI UART, so
  it is not simply a matter of adding a `SECTION RAW` line.

None of this affects booting to the UEFI shell or Linux: the firmware does not
consume the DSDT, it only passes it to the OS.
