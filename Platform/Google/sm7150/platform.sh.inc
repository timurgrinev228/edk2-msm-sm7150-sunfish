# Boot image packaging for Google LAZER devices (Pixel 4a = sunfish).
#
# The Pixel 4a bootloader (Google ABL) refuses / hangs on boot images that
# Qualcomm's own ABL happily accepts.  The only shape it boots is the one used
# by the working U-Boot image of the mobian port:
#
#   header_version 0, page size 4096
#   kernel  @ 0x8000      ramdisk @ 0x1000000
#   second  @ 0xf00000    tags    @ 0x100
#   kernel = gzip(BootShim.bin + UEFI.fd) ++ <device>.dtb   (DTB appended after the gzip stream)
#   a ramdisk must be present (the bootloader ignores its contents)
#
# `mkbootimg` zeroes second_addr whenever second_size == 0, so we patch the
# 4-byte field at offset 28 back to 0x00f00000 afterwards (it is outside the
# image SHA1 `id` hash).
#
# Symptom if you get it wrong:
#   - page_size/header_version != 0/4096  -> "Error verifying the received boot.img: Invalid Parameter"
#   - header_version 2 (or a v2 header claiming a dtb that is not there) -> hangs on the Google logo
#   - `fastboot erase dtbo` is required, otherwise it also hangs on the logo.

function platform_build_bootimg(){
	python3 "${ROOTDIR}/tools/mkbootimg.py" \
		--kernel "${WORKSPACE}/uefi-${DEVICE}-kernel" \
		--ramdisk "${_SUNFISH_RAMDISK:-ramdisk}" \
		--base 0x00000000 \
		--kernel_offset 0x00008000 \
		--ramdisk_offset 0x01000000 \
		--second_offset 0x00f00000 \
		--tags_offset 0x00000100 \
		--pagesize 4096 \
		--header_version 0 \
		--os_version "${BOOTIMG_OS_VERSION}" \
		--os_patch_level "${BOOTIMG_OS_PATCH_LEVEL}" \
		-o "${OUTDIR}/boot-${DEVICE}${EXT}.img" \
		|| return "$?"

	# put the second_addr field back, mkbootimg clears it
	printf '\x00\x00\xf0\x00' | dd of="${OUTDIR}/boot-${DEVICE}${EXT}.img" \
		bs=1 seek=28 conv=notrunc status=none || return "$?"
}
export _SUNFISH_RAMDISK='/mnt/btrfs-1t/@home/batya/uefi-sunfish/edk2-msm/Platform/Google/sm7150/FdtBlob_compat/sunfish-ramdisk.img'
