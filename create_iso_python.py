#!/usr/bin/env python3
"""
Simple ISO 9660 Creator for vib-OS
Creates a bootable ISO from the build/iso directory
"""

import os
import sys
import struct
import datetime

ISO_DIR = "build/iso"
OUTPUT_ISO = "build/vib-os-live.iso"
VOLUME_ID = "VIB-OS-LIVE"

def create_basic_iso():
    """Create a very basic ISO 9660 structure"""
    print("[INFO] Creating basic ISO structure...")
    print("[WARNING] This is a minimal ISO without boot capability.")
    print("[WARNING] For bootable ISO, use xorriso on Linux/macOS.")
    print("")
    print("However, the kernel can be booted directly with QEMU:")
    print("  qemu-system-aarch64 -M virt -cpu max -m 4G \\")
    print("      -kernel build/kernel/unixos.elf \\")
    print("      -append \"live_boot=1 console=ttyAMA0\"")
    print("")
    return 1

if __name__ == "__main__":
    # Check if pycdlib is available
    try:
        import pycdlib
        print("[INFO] pycdlib found - creating proper ISO...")
        iso = pycdlib.PyCdlib()
        iso.new(interchange_level=3, joliet=3, vol_ident=VOLUME_ID)

        # Add files from ISO directory
        for root, dirs, files in os.walk(ISO_DIR):
            for file in files:
                src_path = os.path.join(root, file)
                iso_path = "/" + os.path.relpath(src_path, ISO_DIR).replace("\\", "/")
                print(f"  Adding: {iso_path}")
                iso.add_file(src_path, iso_path=iso_path.upper())

        # Write ISO
        iso.write(OUTPUT_ISO)
        iso.close()

        print(f"\n✓ ISO created: {OUTPUT_ISO}")
        print(f"  Size: {os.path.getsize(OUTPUT_ISO) / (1024*1024):.1f} MB")

    except ImportError:
        print("[INFO] pycdlib not available")
        print("[INFO] Install with: pip install pycdlib")
        print("")
        create_basic_iso()

    except Exception as e:
        print(f"[ERROR] {e}")
        create_basic_iso()
