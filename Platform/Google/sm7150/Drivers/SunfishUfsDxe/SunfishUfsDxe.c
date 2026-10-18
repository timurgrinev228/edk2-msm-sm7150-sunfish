/** @file
  Expose the Pixel 4a UFS host controller to the open EDK2 UFS stack.

  The Qualcomm UFSDxe blobs either reset sunfish (the shared sm7150 one) or
  hang it (the surya one), so UFS is driven by MdeModulePkg instead:

    SunfishUfsDxe                 registers the UFSHCI MMIO window as a
                                  non-discoverable UFS device
    NonDiscoverablePciDeviceDxe   wraps it in EFI_PCI_IO_PROTOCOL
    UfsPciHcDxe                   -> EDKII_UFS_HOST_CONTROLLER_PROTOCOL
    UfsPassThruDxe                -> EXT_SCSI_PASS_THRU
    ScsiBus / ScsiDisk / Partition / Fat

  No clock, regulator or PHY setup is done here.  Google ABL leaves the
  controller fully up (UFS_PHY GDSC on, all UFS clocks running, UFSHCI 2.1
  enabled with the link up and a device present), which is what the
  read-only probe on the device showed.  If that is ever not the case the
  driver backs off rather than touching an unclocked block, which would hang
  the bus.

  Register offsets: drivers/clk/qcom/gcc-sm7150.c and
  arch/arm64/boot/dts/qcom/sm7150.dtsi (ufshc@1d84000) in Linux.

  Copyright (c) 2026, timurgrinev228. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>

#define GCC_BASE                  0x00100000
#define GCC_UFS_PHY_GDSCR         (GCC_BASE + 0x77004)
#define GCC_UFS_PHY_AHB_CBCR      (GCC_BASE + 0x77014)
#define GCC_UFS_PHY_AXI_CBCR      (GCC_BASE + 0x77038)
#define GCC_UFS_PHY_UNIPRO_CBCR   (GCC_BASE + 0x7708C)

#define GDSC_PWR_ON               BIT31
#define CBCR_CLK_OFF              BIT31

#define UFS_HC_BASE               0x01D84000
#define UFS_HC_SIZE               0x00003000
#define UFS_HC_HCS                0x30
#define UFS_HC_HCE                0x34

STATIC
BOOLEAN
SunfishClockRunning (
  IN UINTN  Cbcr
  )
{
  return (MmioRead32 (Cbcr) & CBCR_CLK_OFF) == 0;
}

EFI_STATUS
EFIAPI
SunfishUfsDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (((MmioRead32 (GCC_UFS_PHY_GDSCR) & GDSC_PWR_ON) == 0) ||
      !SunfishClockRunning (GCC_UFS_PHY_AHB_CBCR) ||
      !SunfishClockRunning (GCC_UFS_PHY_AXI_CBCR) ||
      !SunfishClockRunning (GCC_UFS_PHY_UNIPRO_CBCR))
  {
    DEBUG ((DEBUG_ERROR, "SunfishUfs: UFS is powered down or unclocked, not registering it\n"));
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((
    DEBUG_ERROR,
    "SunfishUfs: UFSHCI @%x HCE %x HCS %x\n",
    UFS_HC_BASE,
    MmioRead32 (UFS_HC_BASE + UFS_HC_HCE),
    MmioRead32 (UFS_HC_BASE + UFS_HC_HCS)
    ));

  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeUfs,
             NonDiscoverableDeviceDmaTypeNonCoherent,
             NULL,
             NULL,
             1,
             (UINTN)UFS_HC_BASE,
             (UINTN)UFS_HC_SIZE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SunfishUfs: RegisterNonDiscoverableMmioDevice: %r\n", Status));
  }

  return Status;
}
