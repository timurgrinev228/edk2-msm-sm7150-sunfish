/** @file
  QEMU virt memory map for the sunfish test profile.

  Silicon/Qualcomm/sm7150/Library/PlatformMemoryMapLib describes the real
  device: GICv3 at 0x17A00000/0x17A60000, debug UART at 0x0A88000, 6 GiB of
  DDR starting at 0x80000000.  The PEI MMU setup
  (Silicon/Qualcomm/QcomPkg/Library/MemoryInitPeiLib) maps exactly the regions
  listed there, so running the very same firmware under qemu-system-aarch64
  -M virt (harness: tools/qemu-sunfish/run-fd.sh, profile: sunfishqemu.dsc)
  used to abort on the very first GIC register access - ArmGicDxe reading the
  distributor at 0x08000000, an address the SM7150 map does not contain:

    Synchronous Exception at 0x00000000CA0A82C8
    ESR 0x96000006 (EC 0x25 data abort, DFSC 6: translation fault level 2)
    FAR 0x0000000008000004

  This library keeps the SM7150 table as the single source of truth and adds
  what only exists under QEMU:

    * the GICv3 distributor/redistributor windows and the PL011 console,
    * a trim of the DDR regions to the RAM QEMU was actually started with.
      The firmware describes 6 GiB of DDR, but the harness runs with -m 3072,
      so addresses above 4 GiB are not backed by QEMU at all: reads return 0
      and writes are dropped, which would silently corrupt anything DXE
      allocates there.

  Copyright (c) 2026, sunfish port contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PlatformMemoryMapLib.h>

//
// Reuse the SM7150 table under a different name and then extend it.  Including
// the source instead of copying the table into this file keeps a single source
// of truth: changes to the SM7150 map are picked up by the QEMU profile too.
//
#define GetPlatformMemoryMap  GetSm7150PlatformMemoryMap
#include "../../../../../Silicon/Qualcomm/sm7150/Library/PlatformMemoryMapLib/PlatformMemoryMapLib.c"
#undef GetPlatformMemoryMap

//
// Top of the RAM that qemu-system-aarch64 -M virt provides for -m 3072 (the
// default of tools/qemu-sunfish/run-fd.sh): QEMU's RAM starts at 0x40000000,
// so 3072 MiB of it ends exactly at 0x100000000.  Bump this together with
// run-fd.sh's RAM argument if a bigger guest is wanted.
//
#define QEMU_VIRT_RAM_END  0x100000000ULL

//
// This library takes the shared SM7150 table as-is, so it does not get the
// corrections SunfishMemoryMapLib applies to it.  The one that matters here is
// HLOS 3, which the table gives a length of 0x000E0000 and therefore runs
// 128 KiB into SMEM (0x86000000..0x86200000, the secure partition - both the
// stock and the Mobian device trees put it exactly there).  A QEMU boot log
// shows it plainly as "Memory Allocation 0x00000007 0x85F40000 - 0x8601FFFF".
//
#define QEMU_HLOS3_BASE   0x85F40000ULL
#define QEMU_HLOS3_LENGTH 0x000C0000ULL

//
// The shared table gives two different regions the name "HLOS 6" (0x9DC00000
// and 0x9FFD0000), so the second is unreachable by name.  Renamed here to match
// what SunfishMemoryMapLib does for the device profile.
//
#define QEMU_HLOS6B_BASE  0x9FFD0000ULL
#define QEMU_HLOS6B_NAME  "HLOS 6b"
#define QEMU_NAME_LEN     32

//
// MMIO windows of qemu-system-aarch64 -M virt,gic-version=3:
//
//   GICv3 distributor     0x08000000, 64 KiB
//   GICv3 redistributors  0x080A0000, two 64 KiB frames per CPU; mapped here
//                         for up to 16 CPUs (-smp 8 in the harness)
//   PL011 console         0x09000000, 4 KiB (PcdSerialRegisterBase)
//
STATIC ARM_MEMORY_REGION_DESCRIPTOR_EX  mQemuVirtRegions[] = {
  { "QEMU VIRT GICD", 0x08000000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE },
  { "QEMU VIRT GICR", 0x080A0000, 0x00200000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE },
  { "QEMU VIRT UART", 0x09000000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE },
  { "Terminator",     0,          0,          0,      0,       0,           0,    0         },
};

STATIC ARM_MEMORY_REGION_DESCRIPTOR_EX  mQemuVirtMemoryMap[MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT];

/**
  Return the SM7150 memory map extended with the QEMU virt MMIO windows.

  @return Terminator-counted descriptor table (never NULL).
**/
ARM_MEMORY_REGION_DESCRIPTOR_EX *
GetPlatformMemoryMap (
  VOID
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR_EX  *Sm7150Map;
  UINTN                            Source;
  UINTN                            Index;
  UINTN                            Extra;
  UINT64                           End;

  Sm7150Map = GetSm7150PlatformMemoryMap ();

  //
  // Copy the SM7150 table, terminator included, dropping or trimming the
  // regions that reach past the end of the RAM QEMU provides.
  //
  for (Source = 0, Index = 0; Source < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT - 1; Source++) {
    if (Sm7150Map[Source].Length == 0) {
      break;
    }

    if (Sm7150Map[Source].Address >= QEMU_VIRT_RAM_END) {
      continue;
    }

    ASSERT (Index < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT - 1);

    mQemuVirtMemoryMap[Index] = Sm7150Map[Source];

    //
    // Trim HLOS 3 to end where the secure partition begins.
    //
    if ((Sm7150Map[Source].Address == QEMU_HLOS3_BASE) &&
        (Sm7150Map[Source].Length != QEMU_HLOS3_LENGTH))
    {
      mQemuVirtMemoryMap[Index].Length = QEMU_HLOS3_LENGTH;
    }

    //
    // Disambiguate the second "HLOS 6".
    //
    if (mQemuVirtMemoryMap[Index].Address == QEMU_HLOS6B_BASE) {
      ZeroMem (mQemuVirtMemoryMap[Index].Name, QEMU_NAME_LEN);
      CopyMem (mQemuVirtMemoryMap[Index].Name, QEMU_HLOS6B_NAME,
               sizeof (QEMU_HLOS6B_NAME));
    }

    End = mQemuVirtMemoryMap[Index].Address + mQemuVirtMemoryMap[Index].Length;
    if (End > QEMU_VIRT_RAM_END) {
      mQemuVirtMemoryMap[Index].Length = QEMU_VIRT_RAM_END - Sm7150Map[Source].Address;
    }

    Index++;
  }

  //
  // Append the QEMU-only MMIO windows.  Everything after them is still
  // zero-initialised, which is the terminator the caller loops on.
  //
  for (Extra = 0; mQemuVirtRegions[Extra].Length != 0; Extra++) {
    ASSERT (Index < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT - 1);
    if (Index >= MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT - 1) {
      break;
    }

    mQemuVirtMemoryMap[Index++] = mQemuVirtRegions[Extra];
  }

  return mQemuVirtMemoryMap;
}
