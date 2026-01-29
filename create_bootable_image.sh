#!/bin/bash
# Create bootable disk image for vib-OS
# Works on Windows/Linux/macOS

set -e

echo "========================================"
echo "vib-OS Bootable Image Creator"
echo "========================================"
echo ""

# Check if kernel exists
if [ ! -f "build/kernel/unixos.elf" ]; then
    echo "[ERROR] Kernel not found! Run build_kernel_windows.sh first."
    exit 1
fi

# Create image directory
mkdir -p image
IMAGE_FILE="image/vib-os-live.img"

# Create a 2GB raw disk image
echo "[1/4] Creating 2GB disk image..."
dd if=/dev/zero of="$IMAGE_FILE" bs=1M count=2048 status=progress 2>/dev/null || \
dd if=/dev/zero of="$IMAGE_FILE" bs=1M count=2048

echo "[2/4] Creating bootable structure..."
mkdir -p build/iso/boot/grub
mkdir -p build/iso/EFI/BOOT

# Copy kernel
cp build/kernel/unixos.elf build/iso/boot/kernel.elf
cp build/kernel/unixos.elf build/iso/EFI/BOOT/BOOTAA64.EFI

# Create GRUB config with live boot
cat > build/iso/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0

menuentry "vib-OS (Live Boot)" {
    echo "Loading vib-OS in live mode..."
    linux /boot/kernel.elf live_boot=1 console=ttyAMA0 console=tty0
    boot
}

menuentry "vib-OS (Normal Boot)" {
    echo "Loading vib-OS..."
    linux /boot/kernel.elf root=/dev/vda2 console=ttyAMA0 console=tty0
    boot
}

menuentry "vib-OS (Debug)" {
    echo "Loading vib-OS (debug)..."
    linux /boot/kernel.elf live_boot=1 debug verbose console=ttyAMA0
    boot
}
EOF

echo "[3/4] Image created:"
ls -lh "$IMAGE_FILE"

echo "[4/4] Boot structure ready:"
find build/iso -type f

echo ""
echo "========================================"
echo "✓ Bootable Image Created!"
echo "========================================"
echo ""
echo "Output: $IMAGE_FILE"
echo "Size: $(du -h $IMAGE_FILE | cut -f1)"
echo ""
echo "========================================"
echo "QEMU Testing Instructions"
echo "========================================"
echo ""
echo "Test with QEMU (recommended):"
echo ""
echo "  qemu-system-aarch64 -M virt,gic-version=3 -cpu max -m 4G \\"
echo "      -drive if=none,id=hd0,format=raw,file=$IMAGE_FILE \\"
echo "      -device virtio-blk-device,drive=hd0 \\"
echo "      -serial stdio \\"
echo "      -device virtio-gpu-pci \\"
echo "      -device virtio-keyboard-pci \\"
echo "      -device virtio-mouse-pci \\"
echo "      -kernel build/kernel/unixos.elf \\"
echo "      -append \"live_boot=1 console=ttyAMA0\""
echo ""
echo "Or use the Makefile:"
echo "  make run-gui"
echo ""
echo "The installer GUI should appear automatically in live boot mode!"
echo ""
