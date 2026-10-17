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
# qemu-system-aarch64 -M virt,gic-version=3 memory map
#   GIC distributor      0x08000000
#   GIC redistributors   0x080A0000
#   arch timer PPIs      secure 29 / non-secure 30 / virtual 27
#   PL011 UART           0x09000000
#
[PcdsFixedAtBuild.common]
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x08000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x080A0000
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|29
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|30
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|27
  gArmTokenSpaceGuid.PcdArmArchTimerFreqInHz|62500000
  gQcomTokenSpaceGuid.PcdDebugUartPortBase|0x09000000
