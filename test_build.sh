#!/bin/bash
# Quick test build for installer components

CC="/c/Program Files/LLVM/bin/clang"
CFLAGS="--target=aarch64-unknown-none-elf -Wall -Wextra -Wno-unused-function -ffreestanding -fstack-protector-strong -fno-pic -mcpu=cortex-a72 -O2 -g -Ikernel/include -Ikernel -mgeneral-regs-only -fno-builtin -nostdlib -nostdinc -DARCH_ARM64"

echo "Testing compilation of installer components..."

mkdir -p build/test

echo "[1/10] Compiling boot_params.c..."
"$CC" $CFLAGS -c kernel/core/boot_params.c -o build/test/boot_params.o || exit 1

echo "[2/10] Compiling block_dev.c..."
"$CC" $CFLAGS -c kernel/drivers/block/block_dev.c -o build/test/block_dev.o || exit 1

echo "[3/10] Compiling virtio_block.c..."
"$CC" $CFLAGS -c kernel/drivers/block/virtio_block.c -o build/test/virtio_block.o || exit 1

echo "[4/10] Compiling crc32.c..."
"$CC" $CFLAGS -c kernel/lib/crc32.c -o build/test/crc32.o || exit 1

echo "[5/10] Compiling gpt.c..."
"$CC" $CFLAGS -c kernel/lib/partition/gpt.c -o build/test/gpt.o || exit 1

echo "[6/10] Compiling ext4_mkfs.c..."
"$CC" $CFLAGS -c kernel/fs/ext4_mkfs.c -o build/test/ext4_mkfs.o || exit 1

echo "[7/10] Compiling fat32_simple.c..."
"$CC" $CFLAGS -c kernel/fs/fat32_simple.c -o build/test/fat32_simple.o || exit 1

echo "[8/10] Compiling file_copy.c..."
"$CC" $CFLAGS -c kernel/installer/file_copy.c -o build/test/file_copy.o || exit 1

echo "[9/10] Compiling bootloader.c..."
"$CC" $CFLAGS -c kernel/installer/bootloader.c -o build/test/bootloader.o || exit 1

echo "[10/10] Compiling installer.c..."
"$CC" $CFLAGS -c kernel/apps/installer.c -o build/test/installer.o || exit 1

echo ""
echo "✓ All installer components compiled successfully!"
echo "Generated object files:"
ls -lh build/test/*.o
