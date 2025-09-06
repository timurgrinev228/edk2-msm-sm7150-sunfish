[Defines]
  VENDOR_NAME                    = Xiaomi
  PLATFORM_NAME                  = davinci
  PLATFORM_GUID                  = 3a6218b3-f9fa-48fd-9587-e2b3ab2ae946
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Qualcomm/sm7150/sm7150.fdf
  DEVICE_DXE_FV_COMPONENTS       = Platform/Xiaomi/sm7150/davinci.fdf.inc

!include Platform/Qualcomm/sm7150/sm7150.dsc

[BuildOptions.common]
  GCC:*_*_AARCH64_CC_FLAGS = -DENABLE_SIMPLE_INIT -DENABLE_LINUX_SIMPLE_MASS_STORAGE

[PcdsFixedAtBuild.common]
  gQcomTokenSpaceGuid.PcdMipiFrameBufferWidth|1080
  gQcomTokenSpaceGuid.PcdMipiFrameBufferHeight|2350

  # Simple Init
  gSimpleInitTokenSpaceGuid.PcdGuiDefaultDPI|350

  gRenegadePkgTokenSpaceGuid.PcdDeviceVendor|"Xiaomi"
  gRenegadePkgTokenSpaceGuid.PcdDeviceProduct|"Redmi K20"
  gRenegadePkgTokenSpaceGuid.PcdDeviceCodeName|"davinci"
