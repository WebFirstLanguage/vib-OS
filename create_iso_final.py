#!/usr/bin/env python3
"""
vib-OS ISO Creator using pycdlib
"""

import os
import sys

try:
    import pycdlib
except ImportError:
    print("[ERROR] pycdlib not installed")
    print("Install with: pip install pycdlib")
    sys.exit(1)

ISO_DIR = "build/iso"
OUTPUT_ISO = "build/vib-os-live.iso"
VOLUME_ID = "VIBOS"

def add_directory(iso, local_dir, iso_dir="/"):
    """Recursively add directory to ISO"""
    for item in os.listdir(local_dir):
        local_path = os.path.join(local_dir, item)
        iso_path = iso_dir + "/" + item.upper() if iso_dir != "/" else "/" + item.upper()

        if os.path.isfile(local_path):
            print(f"  Adding file: {iso_path}")
            iso.add_file(local_path, iso_path=iso_path)
        elif os.path.isdir(local_path):
            print(f"  Adding directory: {iso_path}")
            iso.add_directory(iso_path)
            add_directory(iso, local_path, iso_path)

def main():
    print("=" * 50)
    print("vib-OS ISO Creator")
    print("=" * 50)
    print("")

    if not os.path.exists(ISO_DIR):
        print(f"[ERROR] ISO directory not found: {ISO_DIR}")
        return 1

    print(f"[1/3] Initializing ISO...")
    iso = pycdlib.PyCdlib()
    iso.new(interchange_level=4, joliet=3, vol_ident=VOLUME_ID, rock_ridge='1.09')

    print(f"[2/3] Adding files from {ISO_DIR}...")
    add_directory(iso, ISO_DIR)

    print(f"[3/3] Writing ISO to {OUTPUT_ISO}...")
    iso.write(OUTPUT_ISO)
    iso.close()

    size_mb = os.path.getsize(OUTPUT_ISO) / (1024 * 1024)
    print("")
    print("=" * 50)
    print("✓ ISO Created Successfully!")
    print("=" * 50)
    print(f"")
    print(f"Output: {OUTPUT_ISO}")
    print(f"Size: {size_mb:.1f} MB")
    print(f"")
    print("Boot with QEMU:")
    print(f"  qemu-system-aarch64 -M virt -cpu max -m 4G \\")
    print(f"      -cdrom {OUTPUT_ISO} \\")
    print(f"      -kernel build/kernel/unixos.elf \\")
    print(f"      -append \"live_boot=1\"")
    print("")

    return 0

if __name__ == "__main__":
    sys.exit(main())
