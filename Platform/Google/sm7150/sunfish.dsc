[Defines]
  VENDOR_NAME                    = Google
  PLATFORM_NAME                  = sunfish
  PLATFORM_GUID                  = 9d3f7a21-6c4e-4b8a-1e52-7c0d9b3f4a61
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Qualcomm/sm7150/sm7150.fdf
  DEVICE_DXE_FV_COMPONENTS       = Platform/Google/sm7150/sunfish.fdf.inc
  # The shared sm7150 UFSDxe resets sunfish; UFS goes through the open EDK2
  # stack instead (Drivers/SunfishUfsDxe)
  DEFINE DISABLE_UFS             = TRUE

!include Platform/Qualcomm/sm7150/sm7150.dsc

[LibraryClasses.common]
  # Sunfish-specific SoC library variants (Platform/Google/sm7150/Library/)
  PlatformMemoryMapLib|Platform/Google/sm7150/Library/SunfishMemoryMapLib/SunfishMemoryMapLib.inf
  SOCSmbiosInfoLib|Platform/Google/sm7150/Library/SunfishSmbiosInfoLib/SunfishSmbiosInfoLib.inf

[BuildOptions.common]
  GCC:*_*_AARCH64_CC_FLAGS = -DENABLE_SIMPLE_INIT -DENABLE_LINUX_SIMPLE_MASS_STORAGE

[PcdsFixedAtBuild.common]
  gQcomTokenSpaceGuid.PcdMipiFrameBufferWidth|1080
  gQcomTokenSpaceGuid.PcdMipiFrameBufferHeight|2340

  # Simple Init
  gSimpleInitTokenSpaceGuid.PcdGuiDefaultDPI|350

  gRenegadePkgTokenSpaceGuid.PcdDeviceVendor|"Google"
  gRenegadePkgTokenSpaceGuid.PcdDeviceProduct|"Pixel 4a"
  gRenegadePkgTokenSpaceGuid.PcdDeviceCodeName|"sunfish"

  # ARM generic timer PPIs (GIC INTIDs, not PPI indices: ArmGicDxe registers
  # the number as an INTID, and a DT "GIC_PPI n" cell means INTID n + 16).
  #
  # sm7150.dtsi lists them in the arm,armv8-timer order
  # (secure phys, non-secure phys, virtual, hypervisor):
  #
  #     GIC_PPI 1, GIC_PPI 2, GIC_PPI 3, GIC_PPI 0
  #     -> INTID 17, 18, 19, 16
  #
  # Qualcomm does not use the architecturally assigned PPIs 13/14/11/10 here,
  # which is why sdm845/msm8998 set 17/18 as well.  The values that were here
  # (3/2/0) were raw PPI numbers in a different order, i.e. SGIs 0-3: the
  # timer handler would never have fired.
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|17
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|18
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|19
  gArmTokenSpaceGuid.PcdArmArchTimerHypIntrNum|16
[Components.common]
  # UFS through the open EDK2 stack (see Drivers/SunfishUfsDxe)
  Platform/Google/sm7150/Drivers/SunfishUfsDxe/SunfishUfsDxe.inf
  MdeModulePkg/Bus/Pci/NonDiscoverablePciDeviceDxe/NonDiscoverablePciDeviceDxe.inf
  MdeModulePkg/Bus/Pci/UfsPciHcDxe/UfsPciHcDxe.inf
  MdeModulePkg/Bus/Ufs/UfsPassThruDxe/UfsPassThruDxe.inf
