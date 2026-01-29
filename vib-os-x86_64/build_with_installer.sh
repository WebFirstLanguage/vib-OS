#!/bin/bash
# Build vib-OS x86_64 with Installer for VirtualBox

set -e

CC="/c/Program Files/LLVM/bin/clang"
LD="/c/Program Files/LLVM/bin/ld.lld"

CFLAGS="-target x86_64-unknown-none-elf -ffreestanding -fno-stack-protector \
        -fno-stack-check -fno-lto -fno-PIC -m64 -march=x86-64 \
        -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
        -mcmodel=kernel -Ikernel/include -Wall -O2"

BUILD_DIR="build"
ISO_ROOT="iso_root"

echo "=========================================="
echo "vib-OS x86_64 Build with Installer"
echo "=========================================="
echo ""

# Create build directories
mkdir -p "$BUILD_DIR"/{boot,lib,lib/partition,drivers,drivers/block,gui,installer,apps,core,fs,mm,media}

# Compile kernel
echo "[1/6] Compiling core kernel..."
"$CC" $CFLAGS -c kernel/boot/limine_boot.c -o $BUILD_DIR/boot/limine_boot.o
"$CC" $CFLAGS -c kernel/lib/string.c -o $BUILD_DIR/lib/string.o
"$CC" $CFLAGS -c kernel/mm/kmalloc.c -o $BUILD_DIR/mm/kmalloc.o
"$CC" $CFLAGS -c kernel/fs/vfs.c -o $BUILD_DIR/fs/vfs.o
"$CC" $CFLAGS -c kernel/drivers/framebuffer.c -o $BUILD_DIR/drivers/framebuffer.o
"$CC" $CFLAGS -c kernel/drivers/idt.c -o $BUILD_DIR/drivers/idt.o
"$CC" $CFLAGS -c kernel/drivers/acpi.c -o $BUILD_DIR/drivers/acpi.o
"$CC" $CFLAGS -c kernel/drivers/pci.c -o $BUILD_DIR/drivers/pci.o
"$CC" $CFLAGS -c kernel/drivers/ps2.c -o $BUILD_DIR/drivers/ps2.o
"$CC" $CFLAGS -c kernel/drivers/usb.c -o $BUILD_DIR/drivers/usb.o
"$CC" $CFLAGS -c kernel/drivers/usb_xhci.c -o $BUILD_DIR/drivers/usb_xhci.o
"$CC" $CFLAGS -c kernel/drivers/usb_ehci.c -o $BUILD_DIR/drivers/usb_ehci.o
"$CC" $CFLAGS -c kernel/drivers/usb_hid.c -o $BUILD_DIR/drivers/usb_hid.o
"$CC" $CFLAGS -c kernel/media/media.c -o $BUILD_DIR/media/media.o
"$CC" $CFLAGS -c kernel/media/picojpeg.c -o $BUILD_DIR/media/picojpeg.o
"$CC" $CFLAGS -c kernel/media/hd_wallpaper_landscape.c -o $BUILD_DIR/media/wallpaper_land.o
"$CC" $CFLAGS -c kernel/media/hd_wallpaper_nature.c -o $BUILD_DIR/media/wallpaper_nature.o
"$CC" $CFLAGS -c kernel/media/hd_wallpaper_city.c -o $BUILD_DIR/media/wallpaper_city.o
"$CC" $CFLAGS -c kernel/gui/font.c -o $BUILD_DIR/gui/font.o
"$CC" $CFLAGS -c kernel/gui/desktop.c -o $BUILD_DIR/gui/desktop.o
"$CC" $CFLAGS -c kernel/gui/window.c -o $BUILD_DIR/gui/window.o
"$CC" $CFLAGS -c kernel/gui/compositor.c -o $BUILD_DIR/gui/compositor.o
"$CC" $CFLAGS -c kernel/gui/installer_gui_stubs.c -o $BUILD_DIR/gui/gui_stubs.o
"$CC" $CFLAGS -c kernel/mm/mmio.c -o $BUILD_DIR/mm/mmio.o

echo "[2/6] Compiling installer modules..."
"$CC" $CFLAGS -c kernel/core/boot_params.c -o $BUILD_DIR/core/boot_params.o
"$CC" $CFLAGS -c kernel/drivers/block/block_dev.c -o $BUILD_DIR/drivers/block_dev.o
"$CC" $CFLAGS -c kernel/drivers/block/virtio_block.c -o $BUILD_DIR/drivers/virtio_block.o
"$CC" $CFLAGS -c kernel/lib/partition/crc32.c -o $BUILD_DIR/lib/crc32.o
"$CC" $CFLAGS -c kernel/lib/partition/gpt.c -o $BUILD_DIR/lib/gpt.o
"$CC" $CFLAGS -c kernel/fs/ext4_mkfs.c -o $BUILD_DIR/fs/ext4_mkfs.o
"$CC" $CFLAGS -c kernel/fs/fat32_simple.c -o $BUILD_DIR/fs/fat32_simple.o
"$CC" $CFLAGS -c kernel/installer/file_copy.c -o $BUILD_DIR/installer/file_copy.o
"$CC" $CFLAGS -c kernel/installer/bootloader.c -o $BUILD_DIR/installer/bootloader.o
"$CC" $CFLAGS -c kernel/apps/installer.c -o $BUILD_DIR/apps/installer.o

echo "[3/6] Linking kernel with installer..."
"$LD" -nostdlib -static -z max-page-size=0x1000 -T kernel/linker.ld \
    $BUILD_DIR/boot/limine_boot.o \
    $BUILD_DIR/lib/string.o \
    $BUILD_DIR/lib/crc32.o \
    $BUILD_DIR/lib/gpt.o \
    $BUILD_DIR/mm/kmalloc.o \
    $BUILD_DIR/mm/mmio.o \
    $BUILD_DIR/fs/vfs.o \
    $BUILD_DIR/drivers/framebuffer.o \
    $BUILD_DIR/drivers/idt.o \
    $BUILD_DIR/drivers/acpi.o \
    $BUILD_DIR/drivers/pci.o \
    $BUILD_DIR/drivers/ps2.o \
    $BUILD_DIR/drivers/usb.o \
    $BUILD_DIR/drivers/usb_xhci.o \
    $BUILD_DIR/drivers/usb_ehci.o \
    $BUILD_DIR/drivers/usb_hid.o \
    $BUILD_DIR/drivers/block_dev.o \
    $BUILD_DIR/drivers/virtio_block.o \
    $BUILD_DIR/media/media.o \
    $BUILD_DIR/media/picojpeg.o \
    $BUILD_DIR/media/wallpaper_land.o \
    $BUILD_DIR/media/wallpaper_nature.o \
    $BUILD_DIR/media/wallpaper_city.o \
    $BUILD_DIR/gui/font.o \
    $BUILD_DIR/gui/desktop.o \
    $BUILD_DIR/gui/window.o \
    $BUILD_DIR/gui/compositor.o \
    $BUILD_DIR/gui/gui_stubs.o \
    $BUILD_DIR/core/boot_params.o \
    $BUILD_DIR/fs/ext4_mkfs.o \
    $BUILD_DIR/fs/fat32_simple.o \
    $BUILD_DIR/installer/file_copy.o \
    $BUILD_DIR/installer/bootloader.o \
    $BUILD_DIR/apps/installer.o \
    -o $BUILD_DIR/kernel.elf

echo ""
echo "   Kernel: $BUILD_DIR/kernel.elf"
ls -lh $BUILD_DIR/kernel.elf

echo ""
echo "[4/6] Preparing ISO structure..."
rm -rf "$ISO_ROOT"
mkdir -p "$ISO_ROOT"/boot
mkdir -p "$ISO_ROOT"/EFI/BOOT

# Copy kernel
cp "$BUILD_DIR"/kernel.elf "$ISO_ROOT"/boot/kernel.elf
cp "$BUILD_DIR"/kernel.elf "$ISO_ROOT"/EFI/BOOT/BOOTX64.EFI

# Copy Limine files (from limine-bin if available)
if [ -f "limine-bin/limine-bios-cd.bin" ]; then
    cp limine-bin/limine-bios-cd.bin "$ISO_ROOT"/boot/
    cp limine-bin/limine-uefi-cd.bin "$ISO_ROOT"/boot/
fi
# Fall back to iso_root if files exist there
if [ -f "iso_root/boot/limine-bios-cd.bin" ]; then
    cp iso_root/boot/limine-bios-cd.bin "$ISO_ROOT"/boot/
    cp iso_root/boot/limine-uefi-cd.bin "$ISO_ROOT"/boot/
    cp iso_root/boot/limine-bios.sys "$ISO_ROOT"/boot/ 2>/dev/null || true
fi

# Create Limine config with live boot
cat > "$ISO_ROOT"/boot/limine.conf << 'LIMINE_EOF'
timeout: 5

/vib-OS (Live Boot)
    protocol: limine
    kernel_path: boot():/boot/kernel.elf
    cmdline: live_boot=1 console=ttyS0

/vib-OS (Normal Boot)
    protocol: limine
    kernel_path: boot():/boot/kernel.elf
    cmdline: root=/dev/sda2 console=ttyS0

/vib-OS (Debug)
    protocol: limine
    kernel_path: boot():/boot/kernel.elf
    cmdline: live_boot=1 debug verbose
LIMINE_EOF

echo "[5/6] Creating ISO with xorriso..."
if command -v xorriso &> /dev/null; then
    xorriso -as mkisofs \
        -R -J -V "VIB-OS" \
        -b boot/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot boot/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image \
        --protective-msdos-label \
        "$ISO_ROOT" -o vib-os-installer.iso

    echo ""
    echo "=========================================="
    echo "✓ x86_64 ISO with Installer Created!"
    echo "=========================================="
    ls -lh vib-os-installer.iso
else
    echo "[WARNING] xorriso not found - using WSL..."
    wsl bash -c "cd /mnt/g/Logbie/vib-OS/vib-os-x86_64 && xorriso -as mkisofs -R -J -V 'VIB-OS' -b boot/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot boot/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label $ISO_ROOT -o vib-os-installer.iso"

    echo ""
    echo "=========================================="
    echo "✓ x86_64 ISO with Installer Created!"
    echo "=========================================="
    ls -lh vib-os-installer.iso
fi

echo ""
echo "[6/6] ISO ready for VirtualBox!"
echo ""
echo "File: vib-os-x86_64/vib-os-installer.iso"
echo ""
echo "Next: Boot in VirtualBox with EFI enabled"
echo ""
