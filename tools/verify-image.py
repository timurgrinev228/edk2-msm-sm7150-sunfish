#!/usr/bin/env python3
"""Verify that a Pixel 4a boot image really contains the firmware in this tree.

validate-boot-img.py checks the Android header against the ABL rules.  This
script goes one level deeper and answers "is this the firmware I just built?":

    boot image -> gzip payload -> BootShim.bin + SM7150_UEFI.fd
                                        |
                                        +-- DSDT.aml embedded in the FV
                                        +-- appended sunfish DTB

A stale image (built before an ACPI or DSC change) passes the header checks but
fails the content checks here, which is the whole point.

usage:
  tools/verify-image.py <boot.img> [SM7150_UEFI.fd] [DSDT.aml] [dtb]

The optional paths default to the ones in the tree; pass them explicitly when
the build output lives in an isolated -O directory.

exit status: 0 = everything matched, 1 = a check failed, 2 = could not parse.
"""
import hashlib
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

# A UEFI firmware volume starts with a 16-byte zero vector, the file system
# GUID, the FV length and then "_FVH" at +0x28.  EDK2's default is FFS2
# (8c8ce578-8a3d-4f1c-9935-896185c32dd3), but this build uses FFS3
# (5473c07a-3dcb-4dca-bd6f-1e9689e7349a), so look for both.
FFS2_GUID = bytes.fromhex("788cc88c3d8a1c4f9935896185c32dd3")
FFS3_GUID = bytes.fromhex("7ac07354cb3dca4dbd6f1e9689e7349a")
FV_SIG = b"_FVH"
SHIM_SIZE = 0x90  # tools/BootShim/BootShim.bin is a fixed 144-byte stub


def fail(msg):
    print("  [FAIL] %s" % msg)
    return False


def ok(msg):
    print("  [ok]   %s" % msg)
    return True


def warn(msg):
    print("  [warn] %s" % msg)
    return True


def parse_header(data):
    if data[:8] != b"ANDROID!":
        return None
    (k_size, k_addr, r_size, r_addr, s_size, s_addr, tags, page, ver) = \
        struct.unpack_from("<9I", data, 8)
    return dict(k_size=k_size, k_addr=k_addr, r_size=r_size, r_addr=r_addr,
                s_size=s_size, s_addr=s_addr, tags=tags, page=page, ver=ver)


def main():
    img_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "boot-sunfish.img")
    fd_path = sys.argv[2] if len(sys.argv) > 2 else None
    dsdt_path = sys.argv[3] if len(sys.argv) > 3 else \
        os.path.join(ROOT, "Platform/Google/sm7150/AcpiTables/sunfish/DSDT.aml")
    dtb_path = sys.argv[4] if len(sys.argv) > 4 else None

    data = open(img_path, "rb").read()
    hdr = parse_header(data)
    if hdr is None:
        print("not an Android boot image")
        return 2

    good = True
    print("image : %s (%d bytes)" % (img_path, len(data)))
    print("header: version %d, page %d, kernel %d, ramdisk %d"
          % (hdr["ver"], hdr["page"], hdr["k_size"], hdr["r_size"]))

    print("\n=== payload ===")
    page = hdr["page"]
    kernel = data[page:page + hdr["k_size"]]
    if kernel[:2] != b"\x1f\x8b":
        fail("kernel is not gzip (0x%s)" % kernel[:2].hex())
        return 2
    d = zlib.decompressobj(16 + zlib.MAX_WBITS)
    payload = d.decompress(kernel)
    tail = d.unused_data
    good &= ok("gzip -> %d bytes, %d bytes appended" % (len(payload), len(tail)))
    good &= ok("ramdisk %d bytes%s" % (hdr["r_size"], "" if hdr["r_size"] else " (ABL ignores the contents)"))

    print("\n=== firmware volume ===")
    fd_start = -1
    for name, guid in (("FFS3", FFS3_GUID), ("FFS2", FFS2_GUID)):
        # The GUID sits at +0x10 of the FV header (after the 16-byte zero
        # vector), so the FV itself starts 16 bytes before the match.
        off = payload.find(guid)
        if off >= 0x10:
            fd_start = off - 0x10
            good &= ok("%s volume GUID at +0x%x, FV at +0x%x" % (name, off, fd_start))
            break
    if fd_start < 0:
        good &= fail("no firmware volume GUID in the payload")
    else:
        sig = payload[fd_start + 0x28:fd_start + 0x2C]
        fv_len = struct.unpack_from("<Q", payload, fd_start + 0x20)[0]
        good &= (ok if sig == FV_SIG else fail)(
            "FV signature %r at +0x%x, FV length 0x%x"
            % (sig, fd_start + 0x28, fv_len))

        shim = payload[:fd_start]
        shim_file = os.path.join(ROOT, "tools/BootShim/BootShim.bin")
        if os.path.exists(shim_file):
            want = open(shim_file, "rb").read()
            same = shim == want
            good &= (ok if same else fail)(
                "BootShim %d bytes %s tools/BootShim/BootShim.bin"
                % (len(shim), "matches" if same else "does NOT match"))
        elif len(shim) != SHIM_SIZE:
            good &= warn("unexpected BootShim size %d" % len(shim))

        fd = payload[fd_start:]
        if fd_path:
            want = open(fd_path, "rb").read()
            same = fd[:len(want)] == want
            good &= (ok if same else fail)("payload matches %s (%d bytes)" % (fd_path, len(want)))

        if os.path.exists(dsdt_path):
            table = open(dsdt_path, "rb").read()
            hit = fd.find(table)
            if hit >= 0:
                good &= ok("DSDT.aml (%d bytes, md5 %s) embedded at +0x%x"
                           % (len(table), hashlib.md5(table).hexdigest(), hit))
            else:
                # find whatever DSDT is in there, to tell "absent" from "stale"
                j = fd.find(b"DSDT")
                stale = ""
                while j >= 0:
                    if j + 8 <= len(fd):
                        ln = struct.unpack_from("<I", fd, j + 4)[0]
                        if 36 <= ln <= len(fd) - j:
                            stale = " (stale DSDT md5 %s present at +0x%x)" % (
                                hashlib.md5(fd[j:j + ln]).hexdigest(), j)
                            break
                    j = fd.find(b"DSDT", j + 4)
                good &= fail("DSDT.aml (%d bytes, md5 %s) NOT in the firmware%s"
                             % (len(table), hashlib.md5(table).hexdigest(), stale))
        else:
            print("  [skip] %s not found" % dsdt_path)

    print("\n=== appended DTB ===")
    if tail[:4] == bytes.fromhex("d00dfeed"):
        total = struct.unpack_from(">I", tail, 4)[0]
        good &= ok("FDT magic, totalsize %d, version %d" % (total, struct.unpack_from(">I", tail, 20)[0]))
        if b"google,sunfish" in tail:
            good &= ok("DTB is the Pixel 4a device tree (google,sunfish)")
        elif b"qcom,sdmmagpie" in tail:
            # The shared Silicon/ hook appends FdtBlob_compat/<platform>.dtb,
            # which for this port is the generic Qualcomm MTP blob.  Nothing in
            # the UEFI tree parses it, so it only matters to a payload that
            # reuses the DTB later - report it, do not fail the image.
            good &= warn("DTB is the generic SDMMAGPIE blob, not the sunfish one")
        else:
            good &= fail("DTB is neither sunfish nor sdmmagpie")
        if dtb_path:
            want = open(dtb_path, "rb").read()
            good &= (ok if tail[:len(want)] == want else fail)("DTB matches %s" % dtb_path)
    else:
        good &= fail("no appended DTB (tail starts with %r)" % tail[:4])

    print("\n=== RESULT ===")
    print("PASS" if good else "FAIL")
    return 0 if good else 1


if __name__ == "__main__":
    sys.exit(main())
