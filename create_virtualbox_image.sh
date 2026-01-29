#!/bin/bash
# Create VirtualBox-compatible disk image for vib-OS x86_64

set -e

echo "=========================================="
echo "vib-OS VirtualBox Image Creator"
echo "=========================================="
echo ""

# Use the pre-built x86_64 kernel
if [ ! -f "vib-os-x86_64/iso_root/boot/kernel.elf" ]; then
    echo "[ERROR] x86_64 kernel not found!"
    echo "Expected: vib-os-x86_64/iso_root/boot/kernel.elf"
    exit 1
fi

# Create output directory
mkdir -p virtualbox

# Create a 4GB VDI disk image using VBoxManage
OUTPUT_VDI="virtualbox/vib-os-x86_64.vdi"
OUTPUT_IMG="virtualbox/vib-os-x86_64.img"

echo "[1/4] Creating 4GB raw disk image..."
dd if=/dev/zero of="$OUTPUT_IMG" bs=1M count=4096 status=progress 2>/dev/null || \
dd if=/dev/zero of="$OUTPUT_IMG" bs=1M count=4096

echo ""
echo "[2/4] Converting to VDI format..."
if command -v VBoxManage &> /dev/null; then
    VBoxManage convertfromraw "$OUTPUT_IMG" "$OUTPUT_VDI" --format VDI
    echo "✓ VDI created: $OUTPUT_VDI"
else
    echo "⚠ VBoxManage not found - keeping raw image format"
    echo "  You can convert it manually in VirtualBox"
fi

echo ""
echo "[3/4] Preparing boot files..."
mkdir -p virtualbox/boot_files
cp vib-os-x86_64/iso_root/boot/kernel.elf virtualbox/boot_files/
cp vib-os-x86_64/iso_root/boot/limine-bios.sys virtualbox/boot_files/ 2>/dev/null || true
cp vib-os-x86_64/limine.conf virtualbox/boot_files/ 2>/dev/null || true

echo ""
echo "[4/4] Creating setup instructions..."
cat > virtualbox/VIRTUALBOX_SETUP.txt << 'INSTRUCTIONS'
========================================
VirtualBox Setup Instructions
========================================

OPTION A: Use Raw Disk Image
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

1. Open VirtualBox
2. Click "New" to create a new VM
3. Settings:
   - Name: vib-OS
   - Type: Linux
   - Version: Other Linux (64-bit)
   - Memory: 2048 MB (or more)
   - Hard disk: Use existing - select vib-os-x86_64.img

4. Configure VM Settings:
   - System > Processor: Enable PAE/NX
   - System > Acceleration: Enable VT-x/AMD-V
   - Display > Video Memory: 128 MB
   - Display > Graphics Controller: VBoxVGA or VMSVGA

5. Start the VM

OPTION B: Boot from ISO (Recommended)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Unfortunately, ISO creation requires xorriso which
isn't available on Windows.

For ISO creation:
1. Transfer to Linux system
2. Run: cd vib-os-x86_64 && bash build.sh
3. Boot uefi-demo.iso in VirtualBox

OPTION C: Use QEMU (Easiest for Testing)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

QEMU works perfectly on Windows:

1. Install QEMU: https://qemu.weilnetz.de/w64/
2. Run:
   qemu-system-x86_64 -m 2G -serial stdio \
       -kernel vib-os-x86_64/iso_root/boot/kernel.elf

Note: The x86_64 version doesn't have the
installer yet - that's only in the ARM64 build.

========================================
INSTRUCTIONS

echo ""
echo "=========================================="
echo "✓ VirtualBox Files Ready!"
echo "=========================================="
echo ""
echo "Created:"
ls -lh virtualbox/vib-os-x86_64.* 2>/dev/null || ls -lh virtualbox/
echo ""
echo "Instructions: virtualbox/VIRTUALBOX_SETUP.txt"
echo ""
echo "=========================================="
echo "IMPORTANT NOTE"
echo "=========================================="
echo ""
echo "The x86_64 kernel doesn't have the installer"
echo "code yet. The installer is currently only in"
echo "the ARM64 build."
echo ""
echo "For the full installer experience, use QEMU"
echo "with the ARM64 kernel (make run-gui)."
echo ""
echo "To add installer to x86_64, the installer"
echo "code needs to be ported to the x86_64 build."
echo ""
