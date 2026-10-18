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

  Sm7150Map = GetSm7150PlatformMemoryMap ();

  for (Index = 0; Index < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT; Index++) {
    if (Sm7150Map[Index].Length == 0) {
      break;
    }

    mSunfishMemoryMap[Index] = Sm7150Map[Index];

    if ((mSunfishMemoryMap[Index].Address == SUNFISH_HLOS3_BASE) &&
        (mSunfishMemoryMap[Index].Length != SUNFISH_HLOS3_LENGTH))
    {
      mSunfishMemoryMap[Index].Length = SUNFISH_HLOS3_LENGTH;
    }

    if (mSunfishMemoryMap[Index].Address == SUNFISH_HLOS6B_BASE) {
      SunfishSetRegionName (&mSunfishMemoryMap[Index], "HLOS 6b");
    }
  }

  //
  // Whatever was not copied stays zero, and the caller loops on Length == 0.
  //
  while (Index < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT) {
    ZeroMem (&mSunfishMemoryMap[Index], sizeof (mSunfishMemoryMap[Index]));
    Index++;
  }

  return mSunfishMemoryMap;
}