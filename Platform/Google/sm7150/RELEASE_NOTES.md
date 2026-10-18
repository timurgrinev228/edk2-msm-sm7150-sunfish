**Alpha — for testers who can recover their phone from fastboot.**

EDK2 / Renegade UEFI for the Google Pixel 4a 4G (`sunfish`, SM7150 / Snapdragon 730G).

## What works

- Google ABL accepts and starts the image from the `boot` partition.
- PrePi, DXE core and the SoC driver stack (SMEM, DALSys, ChipInfo, CmdDb, NPA, RPMh, PDC, Clock, SCM/TZ, SPMI, TLMM, PMIC, ...) come up.
- BDS runs and the Simple Init boot menu shows on the panel (1080x2340 framebuffer): UEFI Shell, Simple Init, UEFI Boot Menu, Reboot Menu.

## Known limitations

- **No storage.** UFSDxe resets the phone on sunfish, so it is left out (`DISABLE_UFS`). UEFI cannot see the internal UFS yet, so it cannot boot an OS from it.
- Only the boot menu has been tested. USB, touch, buttons, and booting Linux or Windows have not been verified.
- No UART: the only debug output is the framebuffer.

## Install

You need an unlocked bootloader. `fastboot boot` (RAM-only) does not work for unsigned images on the Pixel 4a, so the image has to be flashed.

1. Back up your stock `boot.img` (from the factory image of your build).
2. Reboot to the bootloader (Power + Volume Down), then:

   ```
   fastboot flash boot_a boot-sunfish-<version>.img
   fastboot --set-active=a
   fastboot reboot
   ```

3. To get back to Android, enter the bootloader again (Power + Volume Down) and flash the stock image:

   ```
   fastboot flash boot_a boot.img
   fastboot reboot
   ```

## Image format (for reference)

Android boot image, header v0, page size 4096. The kernel is `gzip(BootShim + SM7150_UEFI.fd)` followed by `sunfish-kernel.dtb`, with a fake arm64 Image header (`text_offset 0x80000`, `flags 0xa`) that ABL requires. `SHA256SUMS` lists the image checksum.
