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