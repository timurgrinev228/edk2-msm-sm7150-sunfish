[Defines]
  VENDOR_NAME                    = Google
  PLATFORM_NAME                  = sunfishqemu
  PLATFORM_GUID                  = b7c4e0f2-9a31-4d5e-8f60-2c1d3e4f5a6b
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Qualcomm/sm7150/sm7150.fdf
  DEVICE_DXE_FV_COMPONENTS       = Platform/Google/sm7150/sunfish.fdf.inc

!include Platform/Google/sm7150/sunfish.dsc

#
# qemu-system-aarch64 -M virt,gic-version=3 layout:
#
#   RAM                 0x40000000, harness default -m 3072 -> ends at 0x100000000
#   GIC distributor     0x08000000
#   GIC redistributors  0x080A0000
#   arch timer PPIs     secure 29 / non-secure 30 / virtual 27
#   PL011 UART          0x09000000
#
# This profile only changes what is address-dependent; the rest of the firmware
# is the very same code that runs on the phone, which is the whole point of the
# test profile.
#
[PcdsFixedAtBuild.common]
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x08000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x080A0000
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|29
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|30
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|27
  gArmTokenSpaceGuid.PcdArmArchTimerFreqInHz|62500000
  gQcomTokenSpaceGuid.PcdDebugUartPortBase|0x09000000

  #
  # Console: the QEMU machine has a PL011, which gives a readable DEBUG log on
  # -serial instead of the framebuffer text renderer.  PcdSerialBaudRate is set
  # explicitly because PL011UartLib divides by it.
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x09000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|115200
  gArmPlatformTokenSpaceGuid.PL011UartClkInHz|24000000

[LibraryClasses.common]
  #
  # The SM7150 map has no window for the QEMU GIC/console and describes 6 GiB of
  # DDR, so this profile uses the variant that appends the QEMU MMIO windows and
  # trims the DDR regions to the RAM QEMU actually provides.
  #
  PlatformMemoryMapLib|Platform/Google/sm7150/Library/QemuVirtMemoryMapLib/QemuVirtMemoryMapLib.inf

  SerialPortLib|ArmPlatformPkg/Library/PL011SerialPortLib/PL011SerialPortLib.inf
  PL011UartLib|ArmPlatformPkg/Library/PL011UartLib/PL011UartLib.inf
  PL011UartClockLib|ArmPlatformPkg/Library/PL011UartClockLib/PL011UartClockLib.inf
