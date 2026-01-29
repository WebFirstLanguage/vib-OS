#!/bin/bash
# Build vib-OS x86_64 with Installer for VirtualBox
# This creates a bootable ISO that includes the live-boot installer

set -e

CC="/c/Program Files/LLVM/bin/clang"
LD="/c/Program Files/LLVM/bin/ld.lld"
OBJCOPY="/c/Program Files/LLVM/bin/llvm-objcopy"

CROSS_TARGET="--target=x86_64-unknown-none-elf"
CFLAGS="-Wall -Wextra -Wno-unused-function -ffreestanding -fstack-protector-strong -fno-pic -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -g -Ikernel/include -Ikernel -fno-builtin -nostdlib -nostdinc -DARCH_X86_64"

echo "=========================================="
echo "vib-OS x86_64 Build with Installer"
echo "=========================================="
echo ""
echo "Target: VirtualBox-compatible ISO"
echo "Architecture: x86_64"
echo ""

# Create build directories
echo "[1/8] Creating build directories..."
mkdir -p build/x86_64/{core,mm,sched,fs,net,syscall,drivers,gui,apps,media,sandbox,sync,ipc,loader,lib,lib/partition,installer,drivers/block,arch/x86_64}
mkdir -p build/x86_64_drivers/{gpu,input,network,usb,nvme,uart,video}

# Build essential kernel modules
echo "[2/8] Compiling core kernel..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/main.c -o build/x86_64/core/main.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/printk.c -o build/x86_64/core/printk.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/process.c -o build/x86_64/core/process.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/boot_config.c -o build/x86_64/core/boot_config.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/boot_params.c -o build/x86_64/core/boot_params.o

echo "[3/8] Compiling memory management..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/mm/pmm.c -o build/x86_64/mm/pmm.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/mm/vmm.c -o build/x86_64/mm/vmm.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/mm/kmalloc.c -o build/x86_64/mm/kmalloc.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/mm/aslr.c -o build/x86_64/mm/aslr.o

echo "[4/8] Compiling filesystems..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/vfs.c -o build/x86_64/fs/vfs.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/ramfs.c -o build/x86_64/fs/ramfs.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/ext4_mkfs.c -o build/x86_64/fs/ext4_mkfs.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/fat32_simple.c -o build/x86_64/fs/fat32_simple.o

echo "[5/8] Compiling installer modules..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/drivers/block/block_dev.c -o build/x86_64/drivers/block/block_dev.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/drivers/block/virtio_block.c -o build/x86_64/drivers/block/virtio_block.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/lib/crc32.c -o build/x86_64/lib/crc32.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/lib/partition/gpt.c -o build/x86_64/lib/partition/gpt.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/installer/file_copy.c -o build/x86_64/installer/file_copy.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/installer/bootloader.c -o build/x86_64/installer/bootloader.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/apps/installer.c -o build/x86_64/apps/installer.o

echo "[6/8] Compiling libraries..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/lib/string.c -o build/x86_64/lib/string.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/lib/stack_protector.c -o build/x86_64/lib/stack_protector.o

echo "[7/8] Compiling architecture-specific (x86_64)..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/arch/x86_64/arch.c -o build/x86_64/arch/x86_64/arch.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/arch/x86_64/apic.c -o build/x86_64/arch/x86_64/apic.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/arch/x86_64/pit.c -o build/x86_64/arch/x86_64/pit.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/arch/x86_64/uart.c -o build/x86_64/arch/x86_64/uart.o
"$AS" $CFLAGS $CROSS_TARGET -c kernel/arch/x86_64/boot.S -o build/x86_64/arch/x86_64/boot.o
"$AS" $CFLAGS $CROSS_TARGET -c kernel/arch/x86_64/switch.S -o build/x86_64/arch/x86_64/switch.o 2>&1

# Check if we have a linker script for x86_64
if [ ! -f "kernel/linker_x86_64.ld" ]; then
    echo "[ERROR] x86_64 linker script not found!"
    echo "Expected: kernel/linker_x86_64.ld"
    echo ""
    echo "The x86_64 build requires a separate linker script."
    echo "You can:"
    echo "  1. Use the vib-os-x86_64/build.sh (Limine-based)"
    echo "  2. Copy linker script from vib-os-x86_64/"
    exit 1
fi

echo "[8/8] Linking kernel..."
"$LD" -nostdlib -static -T kernel/linker_x86_64.ld \
    build/x86_64/arch/x86_64/*.o \
    build/x86_64/core/*.o \
    build/x86_64/mm/*.o \
    build/x86_64/fs/*.o \
    build/x86_64/drivers/block/*.o \
    build/x86_64/lib/*.o \
    build/x86_64/lib/partition/*.o \
    build/x86_64/installer/*.o \
    build/x86_64/apps/*.o \
    -o build/x86_64/vibos-x86_64.elf

echo ""
echo "=========================================="
echo "✓ x86_64 Kernel Built!"
echo "=========================================="
ls -lh build/x86_64/vibos-x86_64.elf
echo ""
