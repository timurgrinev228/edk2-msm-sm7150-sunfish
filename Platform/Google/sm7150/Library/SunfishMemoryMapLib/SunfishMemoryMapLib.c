/** @file
  Pixel 4a (sunfish) memory map.

  Silicon/Qualcomm/sm7150/Library/PlatformMemoryMapLib is shared by every SM7150
  device (davinci, surya, a71, sunfish).  Two defects in its DDR window only
  matter for sunfish, so they are corrected here instead of in the shared table,
  which leaves the Xiaomi/Samsung ports alone.

  1) HLOS 3 overlapped SMEM by 128 KiB.

       HLOS 3  0x85F40000 + 0x000E0000 = 0x86020000
       SMEM    0x86000000 .. 0x86200000

     Every other entry of that DDR window is contiguous with its neighbour, and
     HLOS 3 is the single place where it is not, so the length is off by
     0x20000.  HLOS 3 is AddMem/Conv - ordinary allocatable memory - while SMEM
     is AddMem/Reserv and uncacheable, so the first 128 KiB of SMEM was being
     handed out as free memory while also being described as reserved.  SMEM is
     the region the Qualcomm ConfigLib reads the platform's own data out of, so
     an allocator landing there corrupts it.

     The stock sunfish DTB confirms the layout independently:
     Platform/Google/sm7150/FdtBlob_compat/sunfish.dtb reserves
     sp_region at 0x086000000 size 0x200000, i.e. exactly the SMEM entry, and
     nothing is reserved above 0x85F40000.  Truncating HLOS 3 to
     0x86000000 - 0x85F40000 = 0x000C0000 makes the window continuous.

  2) Two regions were both named "HLOS 6" (0x9DC00000 and 0x9FFD0000).
     LocateMemoryMapAreaByName() returns the first match, so the second was
     unreachable by name.  Nothing in the SM7150 tree looks any of these up by
     name today, so this is a naming fix rather than a behaviour fix; the second
     region is renamed "HLOS 6b".

  This library also adds sunfish-specific reserved memory regions and device
  registers from the Mobian kernel device tree (sm7150-google-sunfish.dts and
  sm7150.dtsi) that are missing from the shared SM7150 table.

  Deliberately left alone: GPU_CC/GPU_CPR and MDSS/DISP_CC overlap as well, but
  those are inherited unchanged from sdm845 (and present in sm8150/sdm670 too),
  both are AddDev/MMAP_IO device windows that the MMU maps identically either
  way, and no SM7150 TRM is available here to confirm the correct extents.

  This keeps the SM7150 table the single source of truth - the shared source is
  #included and renamed rather than copied - so upstream changes still show up
  here.

  Copyright (c) 2026, sunfish port contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PlatformMemoryMapLib.h>

//
// Pull in the shared SM7150 table under a different name, exactly like
// QemuVirtMemoryMapLib does, so both views can coexist in one binary.
//
#define GetPlatformMemoryMap  GetSm7150PlatformMemoryMap
#include "../../../../../Silicon/Qualcomm/sm7150/Library/PlatformMemoryMapLib/PlatformMemoryMapLib.c"
#undef GetPlatformMemoryMap

//
// Length that puts the end of HLOS 3 exactly on the start of SMEM.
//
#define SUNFISH_HLOS3_BASE   0x85F40000ULL
#define SUNFISH_SMEM_BASE    0x86000000ULL
#define SUNFISH_HLOS3_LENGTH (SUNFISH_SMEM_BASE - SUNFISH_HLOS3_BASE)

//
// The two regions that collided on the name "HLOS 6".
//
#define SUNFISH_HLOS6_BASE   0x9DC00000ULL
#define SUNFISH_HLOS6B_BASE  0x9FFD0000ULL

//
// Sunfish-specific device registers and reserved memory from Mobian DT.
// These are NOT in the shared SM7150 table.
//
#define SUNFISH_APSS_SHARED_BASE     0x17C00000ULL
#define SUNFISH_APSS_SHARED_SIZE     0x00001000ULL

#define SUNFISH_APSS_WDT_BASE        0x17C10000ULL
#define SUNFISH_APSS_WDT_SIZE        0x00001000ULL

#define SUNFISH_TIMER_BASE           0x17C20000ULL
#define SUNFISH_TIMER_SIZE           0x00004000ULL  /* 16KB for timer frames */

#define SUNFISH_GICD_BASE            0x17A00000ULL
#define SUNFISH_GICD_SIZE            0x00010000ULL

#define SUNFISH_GICR_BASE            0x17A60000ULL
#define SUNFISH_GICR_SIZE            0x00100000ULL

#define SUNFISH_QUPV3_UART3_BASE     0x0088C000ULL
#define SUNFISH_QUPV3_UART3_SIZE     0x00004000ULL

//
// Reserved memory regions from sunfish DTB (board-level)
//
#define SUNFISH_MPSS_MEM_BASE        0x8B000000ULL
#define SUNFISH_MPSS_MEM_SIZE        0x09800000ULL

#define SUNFISH_VENUS_MEM_BASE       0x94800000ULL
#define SUNFISH_VENUS_MEM_SIZE       0x00500000ULL

#define SUNFISH_CDSP_MEM_BASE        0x94D00000ULL
#define SUNFISH_CDSP_MEM_SIZE        0x01E00000ULL

#define SUNFISH_ADSP_MEM_BASE        0x96B00000ULL
#define SUNFISH_ADSP_MEM_SIZE        0x02200000ULL

#define SUNFISH_WLAN_MSA_MEM_BASE    0x98D00000ULL
#define SUNFISH_WLAN_MSA_MEM_SIZE    0x00200000ULL

#define SUNFISH_NPU_MEM_BASE         0x98F00000ULL
#define SUNFISH_NPU_MEM_SIZE         0x00080000ULL

#define SUNFISH_IPA_FW_MEM_BASE      0x98F80000ULL
#define SUNFISH_IPA_FW_MEM_SIZE      0x00010000ULL

#define SUNFISH_IPA_GSI_MEM_BASE     0x98F90000ULL
#define SUNFISH_IPA_GSI_MEM_SIZE     0x00005000ULL

#define SUNFISH_GPU_MEM_BASE         0x98F95000ULL
#define SUNFISH_GPU_MEM_SIZE         0x00002000ULL

#define SUNFISH_CONT_SPLASH_MEM_BASE 0x9C000000ULL
#define SUNFISH_CONT_SPLASH_MEM_SIZE (1080ULL * 2340ULL * 4ULL)

#define SUNFISH_RAMOOPS_MEM_BASE     0xA481F000ULL
#define SUNFISH_RAMOOPS_MEM_SIZE     0x00400000ULL

#define SUNFISH_RMTFS_MEM_BASE       0xFDE01000ULL
#define SUNFISH_RMTFS_MEM_SIZE       0x00200000ULL

//
// Reserved memory regions from sm7150.dtsi (SoC-level)
//
#define SUNFISH_HYP_MEM_BASE         0x85700000ULL
#define SUNFISH_HYP_MEM_SIZE         0x00600000ULL

#define SUNFISH_XBL_MEM_BASE         0x85D00000ULL
#define SUNFISH_XBL_MEM_SIZE         0x00200000ULL

#define SUNFISH_AOP_MEM_BASE         0x85F00000ULL
#define SUNFISH_AOP_MEM_SIZE         0x00020000ULL

#define SUNFISH_AOP_CMD_DB_MEM_BASE  0x85F20000ULL
#define SUNFISH_AOP_CMD_DB_MEM_SIZE  0x00020000ULL

#define SUNFISH_SEC_APPS_MEM_BASE    0x85FFF000ULL
#define SUNFISH_SEC_APPS_MEM_SIZE    0x00001000ULL

#define SUNFISH_SMEM_MEM_BASE        0x86000000ULL
#define SUNFISH_SMEM_MEM_SIZE        0x00200000ULL

#define SUNFISH_TZ_MEM_BASE          0x86200000ULL
#define SUNFISH_TZ_MEM_SIZE          0x02D00000ULL

#define SUNFISH_CAMERA_MEM_BASE      0x8AB00000ULL
#define SUNFISH_CAMERA_MEM_SIZE      0x00500000ULL

#define SUNFISH_VENUS_MEM2_BASE      0x93400000ULL
#define SUNFISH_VENUS_MEM2_SIZE      0x00500000ULL

#define SUNFISH_CDSP_MEM2_BASE       0x93900000ULL
#define SUNFISH_CDSP_MEM2_SIZE       0x01E00000ULL

#define SUNFISH_ADSP_MEM2_BASE       0x95700000ULL
#define SUNFISH_ADSP_MEM2_SIZE       0x01E00000ULL

#define SUNFISH_WLAN_MSA_MEM2_BASE   0x97500000ULL
#define SUNFISH_WLAN_MSA_MEM2_SIZE   0x00180000ULL

#define SUNFISH_NPU_MEM2_BASE        0x97680000ULL
#define SUNFISH_NPU_MEM2_SIZE        0x00080000ULL

#define SUNFISH_IPA_FW_MEM2_BASE     0x97700000ULL
#define SUNFISH_IPA_FW_MEM2_SIZE     0x00010000ULL

#define SUNFISH_IPA_GSI_MEM2_BASE    0x97710000ULL
#define SUNFISH_IPA_GSI_MEM2_SIZE    0x00005000ULL

#define SUNFISH_GPU_MEM2_BASE        0x97715000ULL
#define SUNFISH_GPU_MEM2_SIZE        0x00002000ULL

#define SUNFISH_QSEECOM_MEM_BASE     0x9E400000ULL
#define SUNFISH_QSEECOM_MEM_SIZE     0x01400000ULL

#define SUNFISH_SEC_CDSP_MEM_BASE    0x9F800000ULL
#define SUNFISH_SEC_CDSP_MEM_SIZE    0x01E00000ULL

#define SUNFISH_DFPS_DATA_MEM_BASE   0x9D700000ULL
#define SUNFISH_DFPS_DATA_MEM_SIZE   0x00100000ULL

STATIC ARM_MEMORY_REGION_DESCRIPTOR_EX  mSunfishMemoryMap[MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT];

/**
  Copy a region name without tripping over the fixed 32-byte field.

  @param[in,out]  Region  Descriptor whose Name is replaced.
  @param[in]      Name    New name, must be shorter than
                          MEMORY_REGION_NAME_MAX_LENGTH.
**/
STATIC
VOID
SunfishSetRegionName (
  IN OUT ARM_MEMORY_REGION_DESCRIPTOR_EX  *Region,
  IN     CONST CHAR8                      *Name
  )
{
  UINTN  Length;

  Length = AsciiStrLen (Name);
  if (Length >= MEMORY_REGION_NAME_MAX_LENGTH) {
    Length = MEMORY_REGION_NAME_MAX_LENGTH - 1;
  }

  ZeroMem (Region->Name, MEMORY_REGION_NAME_MAX_LENGTH);
  CopyMem (Region->Name, Name, Length);
}

/**
  Add a sunfish-specific reserved memory region to the map.

  @param[in,out]  Map      Memory map array.
  @param[in,out]  Index    Current index in the map (updated on return).
  @param[in]      Base     Base address of the region.
  @param[in]      Size     Size of the region.
  @param[in]      Name     Name of the region.
**/
STATIC
VOID
SunfishAddReservedRegion (
  IN OUT ARM_MEMORY_REGION_DESCRIPTOR_EX  *Map,
  IN OUT UINTN                            *Index,
  IN     UINT64                           Base,
  IN     UINT64                           Size,
  IN     CONST CHAR8                      *Name
  )
{
  if (*Index >= MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT - 1) {
    return;
  }

  Map[*Index].Address              = Base;
  Map[*Index].Length               = Size;
  Map[*Index].HobOption            = AddMem;
  Map[*Index].ResourceType         = MEM_RES;
  Map[*Index].ResourceAttribute    = UNCACHEABLE;
  Map[*Index].MemoryType           = Reserv;
  Map[*Index].ArmAttributes        = NS_DEVICE;
  SunfishSetRegionName (&Map[*Index], Name);
  (*Index)++;
}

/**
  Add a sunfish-specific device register region to the map.

  @param[in,out]  Map      Memory map array.
  @param[in,out]  Index    Current index in the map (updated on return).
  @param[in]      Base     Base address of the region.
  @param[in]      Size     Size of the region.
  @param[in]      Name     Name of the region.
**/
STATIC
VOID
SunfishAddDeviceRegion (
  IN OUT ARM_MEMORY_REGION_DESCRIPTOR_EX  *Map,
  IN OUT UINTN                            *Index,
  IN     UINT64                           Base,
  IN     UINT64                           Size,
  IN     CONST CHAR8                      *Name
  )
{
  if (*Index >= MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT - 1) {
    return;
  }

  Map[*Index].Address              = Base;
  Map[*Index].Length               = Size;
  Map[*Index].HobOption            = AddDev;
  Map[*Index].ResourceType         = MMAP_IO;
  Map[*Index].ResourceAttribute    = UNCACHEABLE;
  Map[*Index].MemoryType           = MmIO;
  Map[*Index].ArmAttributes        = NS_DEVICE;
  SunfishSetRegionName (&Map[*Index], Name);
  (*Index)++;
}

/**
  Return the SM7150 memory map with the sunfish-specific corrections applied.

  @return Terminator-counted descriptor table (never NULL).
**/
ARM_MEMORY_REGION_DESCRIPTOR_EX *
GetPlatformMemoryMap (
  VOID
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR_EX  *Sm7150Map;
  UINTN                            Index;
  UINTN                            OutIndex;

  Sm7150Map = GetSm7150PlatformMemoryMap ();

  //
  // Copy the SM7150 table, applying HLOS 3/SMEM fix and HLOS 6b rename.
  //
  for (Index = 0, OutIndex = 0; Index < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT; Index++) {
    if (Sm7150Map[Index].Length == 0) {
      break;
    }

    if (OutIndex >= MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT - 1) {
      break;
    }

    mSunfishMemoryMap[OutIndex] = Sm7150Map[Index];

    if ((mSunfishMemoryMap[OutIndex].Address == SUNFISH_HLOS3_BASE) &&
        (mSunfishMemoryMap[OutIndex].Length != SUNFISH_HLOS3_LENGTH))
    {
      mSunfishMemoryMap[OutIndex].Length = SUNFISH_HLOS3_LENGTH;
    }

    if (mSunfishMemoryMap[OutIndex].Address == SUNFISH_HLOS6B_BASE) {
      SunfishSetRegionName (&mSunfishMemoryMap[OutIndex], "HLOS 6b");
    }

    OutIndex++;
  }

  //
  // Add sunfish-specific device register regions.
  //
  SunfishAddDeviceRegion (mSunfishMemoryMap, &OutIndex,
                          SUNFISH_APSS_SHARED_BASE, SUNFISH_APSS_SHARED_SIZE,
                          "APSS Shared Mbox");
  SunfishAddDeviceRegion (mSunfishMemoryMap, &OutIndex,
                          SUNFISH_APSS_WDT_BASE, SUNFISH_APSS_WDT_SIZE,
                          "APSS Watchdog");
  SunfishAddDeviceRegion (mSunfishMemoryMap, &OutIndex,
                          SUNFISH_TIMER_BASE, SUNFISH_TIMER_SIZE,
                          "ARMv7 Timer Frames");
  SunfishAddDeviceRegion (mSunfishMemoryMap, &OutIndex,
                          SUNFISH_GICD_BASE, SUNFISH_GICD_SIZE,
                          "GICv3 Distributor");
  SunfishAddDeviceRegion (mSunfishMemoryMap, &OutIndex,
                          SUNFISH_GICR_BASE, SUNFISH_GICR_SIZE,
                          "GICv3 Redistributors");
  SunfishAddDeviceRegion (mSunfishMemoryMap, &OutIndex,
                          SUNFISH_QUPV3_UART3_BASE, SUNFISH_QUPV3_UART3_SIZE,
                          "QUPS UART3");

  //
  // Add sunfish-specific reserved memory regions (board-level from DTB).
  //
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_MPSS_MEM_BASE, SUNFISH_MPSS_MEM_SIZE,
                            "MPSS Modem");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_VENUS_MEM_BASE, SUNFISH_VENUS_MEM_SIZE,
                            "Venus Video");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_CDSP_MEM_BASE, SUNFISH_CDSP_MEM_SIZE,
                            "CDSP");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_ADSP_MEM_BASE, SUNFISH_ADSP_MEM_SIZE,
                            "ADSP");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_WLAN_MSA_MEM_BASE, SUNFISH_WLAN_MSA_MEM_SIZE,
                            "WLAN MSA");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_NPU_MEM_BASE, SUNFISH_NPU_MEM_SIZE,
                            "NPU");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_IPA_FW_MEM_BASE, SUNFISH_IPA_FW_MEM_SIZE,
                            "IPA Firmware");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_IPA_GSI_MEM_BASE, SUNFISH_IPA_GSI_MEM_SIZE,
                            "IPA GSI");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_GPU_MEM_BASE, SUNFISH_GPU_MEM_SIZE,
                            "GPU");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_CONT_SPLASH_MEM_BASE, SUNFISH_CONT_SPLASH_MEM_SIZE,
                            "Framebuffer");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_RAMOOPS_MEM_BASE, SUNFISH_RAMOOPS_MEM_SIZE,
                            "Ramoops");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_RMTFS_MEM_BASE, SUNFISH_RMTFS_MEM_SIZE,
                            "RMTFS");

  //
  // Add SoC-level reserved memory regions from sm7150.dtsi.
  //
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_HYP_MEM_BASE, SUNFISH_HYP_MEM_SIZE,
                            "Hypervisor");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_XBL_MEM_BASE, SUNFISH_XBL_MEM_SIZE,
                            "XBL");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_AOP_MEM_BASE, SUNFISH_AOP_MEM_SIZE,
                            "AOP");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_AOP_CMD_DB_MEM_BASE, SUNFISH_AOP_CMD_DB_MEM_SIZE,
                            "AOP Cmd DB");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_SEC_APPS_MEM_BASE, SUNFISH_SEC_APPS_MEM_SIZE,
                            "Secure Apps");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_SMEM_MEM_BASE, SUNFISH_SMEM_MEM_SIZE,
                            "SMEM");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_TZ_MEM_BASE, SUNFISH_TZ_MEM_SIZE,
                            "TrustZone");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_CAMERA_MEM_BASE, SUNFISH_CAMERA_MEM_SIZE,
                            "Camera");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_VENUS_MEM2_BASE, SUNFISH_VENUS_MEM2_SIZE,
                            "Venus Video 2");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_CDSP_MEM2_BASE, SUNFISH_CDSP_MEM2_SIZE,
                            "CDSP 2");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_ADSP_MEM2_BASE, SUNFISH_ADSP_MEM2_SIZE,
                            "ADSP 2");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_WLAN_MSA_MEM2_BASE, SUNFISH_WLAN_MSA_MEM2_SIZE,
                            "WLAN MSA 2");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_NPU_MEM2_BASE, SUNFISH_NPU_MEM2_SIZE,
                            "NPU 2");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_IPA_FW_MEM2_BASE, SUNFISH_IPA_FW_MEM2_SIZE,
                            "IPA Firmware 2");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_IPA_GSI_MEM2_BASE, SUNFISH_IPA_GSI_MEM2_SIZE,
                            "IPA GSI 2");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_GPU_MEM2_BASE, SUNFISH_GPU_MEM2_SIZE,
                            "GPU 2");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_QSEECOM_MEM_BASE, SUNFISH_QSEECOM_MEM_SIZE,
                            "QSEECOM");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_SEC_CDSP_MEM_BASE, SUNFISH_SEC_CDSP_MEM_SIZE,
                            "Secure CDSP");
  SunfishAddReservedRegion (mSunfishMemoryMap, &OutIndex,
                            SUNFISH_DFPS_DATA_MEM_BASE, SUNFISH_DFPS_DATA_MEM_SIZE,
                            "DFPS Data");

  //
  // Whatever was not copied stays zero, and the caller loops on Length == 0.
  //
  while (OutIndex < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT) {
    ZeroMem (&mSunfishMemoryMap[OutIndex], sizeof (mSunfishMemoryMap[OutIndex]));
    OutIndex++;
  }

  return mSunfishMemoryMap;
}