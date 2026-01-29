#!/bin/bash
# Complete installer test build script for Windows

set -e

CC="/c/Program Files/LLVM/bin/clang"
LD="/c/Program Files/LLVM/bin/ld.lld"
OBJCOPY="/c/Program Files/LLVM/bin/llvm-objcopy"

CFLAGS="--target=aarch64-unknown-none-elf -Wall -Wextra -Wno-unused-function -ffreestanding -fstack-protector-strong -fno-pic -mcpu=cortex-a72 -O2 -g -Ikernel/include -Ikernel -mgeneral-regs-only -fno-builtin -nostdlib -nostdinc -DARCH_ARM64"

echo "========================================"
echo "vib-OS Installer Test Build"
echo "========================================"
echo ""

# Create build directories
mkdir -p build/installer_test

echo "Compiling installer components..."

# Core installer files
"$CC" $CFLAGS -c kernel/core/boot_params.c -o build/installer_test/boot_params.o
"$CC" $CFLAGS -c kernel/drivers/block/block_dev.c -o build/installer_test/block_dev.o
"$CC" $CFLAGS -c kernel/drivers/block/virtio_block.c -o build/installer_test/virtio_block.o
"$CC" $CFLAGS -c kernel/lib/crc32.c -o build/installer_test/crc32.o
"$CC" $CFLAGS -c kernel/lib/partition/gpt.c -o build/installer_test/gpt.o
"$CC" $CFLAGS -c kernel/fs/ext4_mkfs.c -o build/installer_test/ext4_mkfs.o
"$CC" $CFLAGS -c kernel/fs/fat32_simple.c -o build/installer_test/fat32_simple.o
"$CC" $CFLAGS -c kernel/installer/file_copy.c -o build/installer_test/file_copy.o
"$CC" $CFLAGS -c kernel/installer/bootloader.c -o build/installer_test/bootloader.o
"$CC" $CFLAGS -c kernel/apps/installer.c -o build/installer_test/installer.o

# Supporting kernel files needed for linking
"$CC" $CFLAGS -c kernel/lib/string.c -o build/installer_test/string.o
"$CC" $CFLAGS -c kernel/core/printk.c -o build/installer_test/printk.o
"$CC" $CFLAGS -c kernel/mm/kmalloc.c -o build/installer_test/kmalloc.o

echo ""
echo "✓ Compilation successful!"
echo ""
echo "Generated object files:"
ls -lh build/installer_test/*.o | awk '{print $9, $5}'

echo ""
echo "========================================"
echo "Installer module size:"
SIZE=$(du -sh build/installer_test | awk '{print $1}')
echo "$SIZE"
echo "========================================"
echo ""
echo "✓ All installer components built successfully!"
echo "  The installer is ready to be integrated into the kernel."
