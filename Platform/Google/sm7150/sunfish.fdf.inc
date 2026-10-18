// per-device BSP DXEs for Pixel 4a (sunfish)
// структура взята с Xiaomi surya - тот же чип SM7150

FILE DRIVER = cd823a4d-7dec-4531-ae5d-4134fa4127b8 {
    SECTION DXE_DEPEX = Platform/EFI_Binaries/Drivers/sm7150/UsbConfigDxe/UsbConfigDxe.depex
    SECTION PE32 = Platform/EFI_Binaries/Drivers/sm7150/UsbConfigDxe/UsbConfigDxe.efi
    SECTION UI = "UsbConfigDxe"
  }

  FILE DRIVER = 11faed4c-b21f-4d88-8e48-c4c28a1e50df {
    SECTION DXE_DEPEX = Platform/EFI_Binaries/Drivers/sm7150/UsbPwrCtrlDxe/UsbPwrCtrlDxe.depex
    SECTION PE32 = Platform/EFI_Binaries/Drivers/sm7150/UsbPwrCtrlDxe/UsbPwrCtrlDxe.efi
    SECTION UI = "UsbPwrCtrlDxe"
  }

  FILE DRIVER = 94f8a6a7-dc34-4101-88c1-99179cceae83 {
    SECTION DXE_DEPEX = Platform/EFI_Binaries/Drivers/sm7150/UsbfnDwc3Dxe/UsbfnDwc3Dxe.depex
    SECTION PE32 = Platform/EFI_Binaries/Drivers/sm7150/UsbfnDwc3Dxe/UsbfnDwc3Dxe.efi
    SECTION UI = "UsbfnDwc3Dxe"
  }

// ACPI Tables
FILE FREEFORM = 7E374E25-8E01-4FEE-87F2-390C23C606CD {
# 	Customized DSDT
    SECTION RAW = Platform/Google/sm7150/AcpiTables/sunfish/DSDT.aml
# 	Common Tables
	SECTION RAW = Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/bgrt.acpi
	SECTION RAW = Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/Facp.acpi
	SECTION RAW = Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/Facs.acpi
	SECTION RAW = Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/Gtdt.acpi
	SECTION RAW = Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/Madt.acpi
	SECTION RAW = Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/Mcfg.acpi
	SECTION RAW = Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/Pptt.acpi
	SECTION RAW = Silicon/Qualcomm/sm7150/AcpiTables/BuiltIn/TPM2.acpi
	SECTION UI = "AcpiTables"
}

// UFS: ABL leaves the controller up; register it as a non-discoverable
// device and let the open EDK2 UFS stack drive it.
INF Platform/Google/sm7150/Drivers/SunfishUfsDxe/SunfishUfsDxe.inf
INF MdeModulePkg/Bus/Pci/NonDiscoverablePciDeviceDxe/NonDiscoverablePciDeviceDxe.inf
INF MdeModulePkg/Bus/Pci/UfsPciHcDxe/UfsPciHcDxe.inf
INF MdeModulePkg/Bus/Ufs/UfsPassThruDxe/UfsPassThruDxe.inf
