# Mobian Kernel Device Tree Reference for Pixel 4a (sunfish)

Source: `/mnt/btrfs-1t/@home/batya/projects/mobian-pixel4a/linux-fork/arch/arm64/boot/dts/qcom/`
- `sm7150-google-sunfish.dts` - Board-level DT
- `sm7150.dtsi` - SoC-level DT
- `pm6150.dtsi` / `pm6150l.dtsi` - PMIC DTs

---

## 1. Memory Map & Reserved Memory Regions

From `sm7150-google-sunfish.dts` (reserved-memory section):

| Region | Address | Size | Description |
|--------|---------|------|-------------|
| `mpss_mem` | 0x8B000000 | 0x9800000 | Modem |
| `venus_mem` | 0x94800000 | 0x500000 | Video |
| `cdsp_mem` | 0x94D00000 | 0x1E00000 | Compute DSP |
| `adsp_mem` | 0x96B00000 | 0x2200000 | Audio DSP |
| `wlan_msa_mem` | 0x98D00000 | 0x200000 | WiFi MSA |
| `npu_mem` | 0x98F00000 | 0x80000 | NPU |
| `ipa_fw_mem` | 0x98F80000 | 0x10000 | IPA firmware |
| `ipa_gsi_mem` | 0x98F90000 | 0x5000 | IPA GSI |
| `gpu_mem` | 0x98F95000 | 0x2000 | GPU |
| `cont_splash_mem` | 0x9C000000 | 1080×2340×4 | Framebuffer |
| `ramoops_mem` | 0xA481F000 | 0x400000 | ramoops (panic logs) |
| `rmtfs_mem` | 0xFDE01000 | 0x200000 | RMTFS |

From `sm7150.dtsi` (SoC-level reserved-memory):

| Region | Address | Size | Description |
|--------|---------|------|-------------|
| `hyp_mem` | 0x85700000 | 0x600000 | Hypervisor |
| `xbl_mem` | 0x85D00000 | 0x200000 | XBL |
| `aop_mem` | 0x85F00000 | 0x20000 | AOP |
| `aop_cmd_db_mem` | 0x85F20000 | 0x20000 | AOP cmd DB |
| `sec_apps_mem` | 0x85FFF000 | 0x1000 | Secure apps |
| `smem_mem` | 0x86000000 | 0x200000 | SMEM |
| `tz_mem` | 0x86200000 | 0x2D00000 | TrustZone |
| `camera_mem` | 0x8AB00000 | 0x500000 | Camera |
| `mpss_mem` | 0x8B000000 | 0x8400000 | Modem (different size!) |
| `venus_mem` | 0x93400000 | 0x500000 | Video |
| `cdsp_mem` | 0x93900000 | 0x1E00000 | CDSP |
| `adsp_mem` | 0x95700000 | 0x1E00000 | ADSP |
| `wlan_msa_mem` | 0x97500000 | 0x180000 | WiFi MSA |
| `npu_mem` | 0x97680000 | 0x80000 | NPU |
| `ipa_fw_mem` | 0x97700000 | 0x10000 | IPA firmware |
| `ipa_gsi_mem` | 0x97710000 | 0x5000 | IPA GSI |
| `gpu_mem` | 0x97715000 | 0x2000 | GPU |
| `qseecom_mem` | 0x9E400000 | 0x1400000 | QSEECOM |
| `sec_cdsp_mem` | 0x9F800000 | 0x1E00000 | Secure CDSP |
| `dfps_data_mem` | 0x9D700000 | 0x100000 | DFPS data |

**Main RAM**: `memory@80000000` - device_type = "memory", reg = `<0x0 0x80000000 0x0 0x0>` (size filled by bootloader)

---

## 2. GICv3 Interrupt Controller

From `sm7150.dtsi`:
```
intc: interrupt-controller@17a00000 {
    compatible = "arm,gic-v3";
    reg = <0x0 0x17a00000 0x0 0x10000>,  /* GICD */
          <0x0 0x17a60000 0x0 0x100000>; /* GICR * 8 */
    interrupts = <GIC_PPI 9 IRQ_TYPE_LEVEL_HIGH>;
    #interrupt-cells = <3>;
    interrupt-controller;
}
```

**Matches current UEFI**: GICD at 0x17A00000, GICR at 0x17A60000 ✓

---

## 3. Timer

From `sm7150.dtsi`:
```
timer@17c20000 {
    compatible = "arm,armv7-timer-mem";
    reg = <0x0 0x17c20000 0x0 0x1000>;
    ranges = <0 0 0 0x20000000>;
    #address-cells = <1>;
    #size-cells = <1>;
    frame@17c21000 {
        reg = <0x17c21000 0x1000>, <0x17c22000 0x1000>;
        interrupts = <GIC_SPI 8 IRQ_TYPE_LEVEL_HIGH>,  /* CNTPSIRQ */
                     <GIC_SPI 6 IRQ_TYPE_LEVEL_HIGH>;  /* CNTPNSIRQ */
        frame-number = <0>;
    };
    frame@17c23000 {
        reg = <0x17c23000 0x1000>;
        interrupts = <GIC_SPI 7 IRQ_TYPE_LEVEL_HIGH>;  /* CNTVIRQ */
        frame-number = <1>;
    };
};
```

**ARMv8 Generic Timer** (PPI):
```
interrupts = <GIC_PPI 1 IRQ_TYPE_LEVEL_LOW>,  /* EL1 virtual timer */
             <GIC_PPI 2 IRQ_TYPE_LEVEL_LOW>,  /* EL1 non-secure phys */
             <GIC_PPI 3 IRQ_TYPE_LEVEL_LOW>,  /* EL1 secure phys */
             <GIC_PPI 0 IRQ_TYPE_LEVEL_LOW>;  /* EL2 virtual timer */
```

**Current UEFI**: Uses `PcdArmArchTimerFreqInHz = 19200000`, `PcdArmArchTimerSecIntrNum = 17`, `PcdArmArchTimerIntrNum = 18` - **NEEDS UPDATE** to match PPI mapping

---

## 4. UART / Console

### Debug UART (Geni/QUPV3) - UART3
```
uart3: serial@88c000 {
    compatible = "qcom,geni-uart";
    reg = <0x0 0x0088c000 0x0 0x4000>;
    interrupts = <GIC_SPI 604 IRQ_TYPE_LEVEL_HIGH>;
}
```
Used for Bluetooth in Linux.

### Serial Console - UART8
```
aliases {
    serial0 = &uart8;
};
chosen {
    stdout-path = "serial0:115200n8";
};
```
UART8 is the main console at 115200 baud.

**Current UEFI**: Uses FrameBufferSerialPortLib by default, PL011 (0x09000000) for QEMU. **Could add Geni UART support.**

---

## 5. Watchdog

From `sm7150.dtsi`:
```
watchdog@17c10000 {
    compatible = "qcom,apss-wdt-sm7150", "qcom,kpss-wdt";
    reg = <0x0 0x17c10000 0x0 0x1000>;
    clocks = <&sleep_clk>;  /* 32 kHz */
    interrupts = <GIC_SPI 0 IRQ_TYPE_EDGE_RISING>;
}
```

**Current UEFI**: Uses `PcdDebugUartPortBase = 0xA88000` for watchdog - **WRONG ADDRESS**

---

## 6. APSS Shared Mailbox

```
apss_shared: mailbox@17c00000 {
    compatible = "qcom,sm7150-apss-shared", "qcom,sdm845-apss-shared";
    reg = <0x0 0x17c00000 0x0 0x1000>;
    #mbox-cells = <1>;
};
```

---

## 7. Display / Framebuffer

From `sm7150-google-sunfish.dts`:
```
framebuffer@9c000000 {
    compatible = "simple-framebuffer";
    reg = <0x0 0x9c000000 0x0 (1080 * 2340 * 4)>;
    width = <1080>;
    height = <2340>;
    stride = <(1080 * 4)>;
    format = "a8r8g8b8";
    clocks = <&gcc GCC_DISP_HF_AXI_CLK>;
};
```

**Current UEFI**: `PcdMipiFrameBufferAddress = 0x9C000000`, Width=1080, Height=2340 ✓

---

## 8. UFS Storage

From `sm7150-google-sunfish.dts`:
```
&ufs_mem_hc {
    reset-gpios = <&tlmm 119 GPIO_ACTIVE_LOW>;
    vcc-supply = <&vreg_l19a_2p85>;
    vcc-max-microamp = <600000>;
    vccq2-supply = <&vreg_l12a_1p8>;
    vccq2-max-microamp = <600000>;
    status = "okay";
};
&ufs_mem_phy {
    vdda-phy-supply = <&vdda_ufs_core>;
    vdda-pll-supply = <&vdda_ufs_1p2>;
    status = "okay";
};
```

UFS controller is in the SoC DT (`sm7150.dtsi`) - need to check base address.

---

## 9. CPU Topology

- **Cluster 0 (Little)**: 6x Cortex-A55 (cpu0-cpu5) @ up to 1.8 GHz
- **Cluster 1 (Big)**: 2x Cortex-A76 (cpu6-cpu7) @ up to 2.3 GHz
- **Total**: 8 cores

**Current UEFI**: `PcdCoreCount = 8`, `PcdClusterCount = 2` ✓

---

## 10. Clocks

- **XO Board**: 38.4 MHz (`xo_board`)
- **Sleep Clock**: 32 kHz (`sleep_clk`)
- **ARM Arch Timer**: 19.2 MHz (half of XO) - current UEFI uses 19.2 MHz ✓

---

## 11. PMIC (PM6150 + PM6150L)

Key regulators used by sunfish:
- `vph_pwr` - 3.7V fixed
- `vreg_bob` - 3.3-4.0V (BOB buck)
- `vreg_s8c_1p2` - 1.2-1.73V (SMPS8)
- `vreg_l13a_1p8` - 1.7-1.86V (LDO13) - Display VDDIO
- `vreg_l7c_3p0` - 3.0V (LDO7) - Display VDDA
- `vreg_l19a_2p85` - 2.9-3.3V (LDO19) - UFS VCC
- `vreg_l12a_1p8` - 1.7-1.95V (LDO12) - UFS VCCQ2
- `camera_ldo` - 2.85V (PM6150L GPIO12) - Rear camera VCM

---

## 12. Key Differences from Current UEFI Port

| Item | Current UEFI | Mobian DT | Action |
|------|-------------|-----------|--------|
| Timer IRQs | 17 (sec), 18 (non-sec) | PPI 1/2/3/0 | Update PCDs |
| Watchdog base | 0xA88000 | 0x17C10000 | Fix address |
| Timer frame | Not mapped | 0x17C20000 | Add to memory map |
| GIC | 0x17A00000/0x17A60000 | Same | ✓ |
| Framebuffer | 0x9C000000, 1080×2340 | Same | ✓ |
| Reserved regions | Minimal | Extensive | Update memory map |
| UART console | Framebuffer / PL011 | UART8 (QUPS) | Add Geni UART |

---

## 13. Memory Map Improvements Needed

The current `SunfishMemoryMapLib` should be updated to include:

1. **Timer frames** (0x17C20000-0x17C24000) - ARMv7 timer memory-mapped registers
2. **Watchdog** (0x17C10000) - QCOM APSS WDT
3. **APSS Shared** (0x17C00000) - Mailbox
4. **All reserved memory regions** from both DTs
5. **Correct SMEM size** - Board DT says 0x200000, SoC DT says 0x200000 ✓
6. **Remove overlap** between HLOS 3 and SMEM (already fixed in SunfishMemoryMapLib)

---

## 14. Boot Parameters from Chosen Node

```
chosen {
    stdout-path = "serial0:115200n8";
    framebuffer@9c000000 { ... };
};
```

The kernel expects:
- Console on UART8 at 115200n8
- Framebuffer at 0x9C000000, 1080×2340, ARGB8888

---

## 15. Files to Update in UEFI Port

1. **Platform/Google/sm7150/Library/SunfishMemoryMapLib/** - Add missing regions
2. **Silicon/Qualcomm/sm7150/Library/PlatformPrePiLib/PlatformUtils.c** - Fix watchdog address
3. **Silicon/Qualcomm/QcomPkg/QcomCommonDsc.inc** - Add Geni UART support
4. **Platform/Google/sm7150/sunfish.dsc** - Update PCDs for timer IRQs
5. **Platform/Google/sm7150/AcpiTables/sunfish/DSDT.aml** - May need updates for new devices