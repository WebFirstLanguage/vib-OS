#!/bin/bash
# Complete kernel build script for Windows
# Works around Makefile path issues on MSYS2/MinGW

set -e

CC="/c/Program Files/LLVM/bin/clang"
AS="/c/Program Files/LLVM/bin/clang"
LD="/c/Program Files/LLVM/bin/ld.lld"

CROSS_TARGET="--target=aarch64-unknown-none-elf"
CFLAGS="-Wall -Wextra -Wno-unused-function -ffreestanding -fstack-protector-strong -fno-pic -mcpu=cortex-a72 -O2 -g -Ikernel/include -Ikernel -mgeneral-regs-only -fno-builtin -nostdlib -nostdinc -DARCH_ARM64"
CFLAGS_MEDIA="-Wall -Wextra -Wno-unused-function -ffreestanding -fstack-protector-strong -fno-pic -mcpu=cortex-a72 -O2 -g -Ikernel/include -Ikernel -fno-builtin -nostdlib -nostdinc -DARCH_ARM64"

echo "========================================"
echo "vib-OS Kernel Build for Windows"
echo "========================================"
echo ""

# Create build directories
echo "[SETUP] Creating build directories..."
mkdir -p build/kernel/{core,mm,sched,fs,net,syscall,drivers,gui,apps,media,sandbox,sync,ipc,loader,lib,lib/partition,installer,drivers/block,arch/arm64,assets}
mkdir -p build/drivers/{gpu,input,network,usb,nvme,uart,video}

# Compile kernel sources
echo "[BUILD] Compiling kernel sources..."

# Core
echo "  [1/15] Core..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/main.c -o build/kernel/core/main.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/printk.c -o build/kernel/core/printk.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/process.c -o build/kernel/core/process.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/boot_config.c -o build/kernel/core/boot_config.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/core/boot_params.c -o build/kernel/core/boot_params.o

# Memory Management
echo "  [2/15] Memory Management..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/mm/pmm.c -o build/kernel/mm/pmm.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/mm/vmm.c -o build/kernel/mm/vmm.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/mm/kmalloc.c -o build/kernel/mm/kmalloc.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/mm/aslr.c -o build/kernel/mm/aslr.o

# Scheduler
echo "  [3/15] Scheduler..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/sched/sched.c -o build/kernel/sched/sched.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/sched/fork.c -o build/kernel/sched/fork.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/sched/signal.c -o build/kernel/sched/signal.o

# Filesystem
echo "  [4/15] Filesystems..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/vfs.c -o build/kernel/fs/vfs.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/vfs_compat.c -o build/kernel/fs/vfs_compat.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/ramfs.c -o build/kernel/fs/ramfs.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/ext4.c -o build/kernel/fs/ext4.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/apfs.c -o build/kernel/fs/apfs.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/ext4_mkfs.c -o build/kernel/fs/ext4_mkfs.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/fat32_simple.c -o build/kernel/fs/fat32_simple.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/fs/fat32.c -o build/kernel/fs/fat32.o

# Network
echo "  [5/15] Networking..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/net/tcp_ip.c -o build/kernel/net/tcp_ip.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/net/socket.c -o build/kernel/net/socket.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/net/dns.c -o build/kernel/net/dns.o

# System calls
echo "  [6/15] System calls..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/syscall/syscall.c -o build/kernel/syscall/syscall.o

# Kernel drivers
echo "  [7/15] Kernel drivers..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/drivers/pci/pci.c -o build/kernel/drivers/pci.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/drivers/audio/intel_hda.c -o build/kernel/drivers/intel_hda.o

# Block devices
echo "  [8/15] Block devices..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/drivers/block/block_dev.c -o build/kernel/drivers/block/block_dev.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/drivers/block/virtio_block.c -o build/kernel/drivers/block/virtio_block.o

# GUI
echo "  [9/15] GUI system..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/gui/window.c -o build/kernel/gui/window.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/gui/desktop.c -o build/kernel/gui/desktop.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/gui/terminal.c -o build/kernel/gui/terminal.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/gui/font.c -o build/kernel/gui/font.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/gui/app.c -o build/kernel/gui/app.o

# Apps
echo "  [10/15] Applications..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/apps/embedded_apps.c -o build/kernel/apps/embedded_apps.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/apps/launcher.c -o build/kernel/apps/launcher.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/apps/installer.c -o build/kernel/apps/installer.o

# Media (without -mgeneral-regs-only for FP support)
echo "  [11/15] Media decoders..."
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/media.c -o build/kernel/media/media.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/picojpeg.c -o build/kernel/media/picojpeg.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/tpng.c -o build/kernel/media/tpng.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/seed_assets.c -o build/kernel/media/seed_assets.o

# Media assets (large files)
echo "  [12/15] Media assets..."
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/bootstrap_landscape_jpg.c -o build/kernel/media/bootstrap_landscape_jpg.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/bootstrap_portrait_jpg.c -o build/kernel/media/bootstrap_portrait_jpg.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/bootstrap_square_jpg.c -o build/kernel/media/bootstrap_square_jpg.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/bootstrap_wallpaper_jpg.c -o build/kernel/media/bootstrap_wallpaper_jpg.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/bootstrap_nature_jpg.c -o build/kernel/media/bootstrap_nature_jpg.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/bootstrap_city_jpg.c -o build/kernel/media/bootstrap_city_jpg.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/bootstrap_httpbin_jpg.c -o build/kernel/media/bootstrap_httpbin_jpg.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/bootstrap_test_png.c -o build/kernel/media/bootstrap_test_png.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/hd_wallpaper_landscape.c -o build/kernel/media/hd_wallpaper_landscape.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/hd_wallpaper_nature.c -o build/kernel/media/hd_wallpaper_nature.o
"$CC" $CFLAGS_MEDIA $CROSS_TARGET -c kernel/media/hd_wallpaper_city.c -o build/kernel/media/hd_wallpaper_city.o

# Sandbox
echo "  [13/15] Sandbox & Libraries..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/sandbox/sandbox.c -o build/kernel/sandbox/sandbox.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/sync/spinlock.c -o build/kernel/sync/spinlock.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/ipc/pipe.c -o build/kernel/ipc/pipe.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/loader/elf.c -o build/kernel/loader/elf.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/lib/string.c -o build/kernel/lib/string.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/lib/stack_protector.c -o build/kernel/lib/stack_protector.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/lib/crc32.c -o build/kernel/lib/crc32.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/lib/partition/gpt.c -o build/kernel/lib/partition/gpt.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/assets/icons.c -o build/kernel/assets/icons.o

# Installer
echo "  [14/15] Installer..."
"$CC" $CFLAGS $CROSS_TARGET -c kernel/installer/file_copy.c -o build/kernel/installer/file_copy.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/installer/bootloader.c -o build/kernel/installer/bootloader.o

# Architecture-specific (ARM64)
echo "  [15/15] ARM64 architecture..."
"$AS" $CFLAGS $CROSS_TARGET -c kernel/arch/arm64/boot.S -o build/kernel/arch/arm64/boot.o
"$AS" $CFLAGS $CROSS_TARGET -c kernel/arch/arm64/switch.S -o build/kernel/arch/arm64/switch.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/arch/arm64/arch.c -o build/kernel/arch/arm64/arch.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/arch/arm64/gic.c -o build/kernel/arch/arm64/gic.o
"$CC" $CFLAGS $CROSS_TARGET -c kernel/arch/arm64/timer.c -o build/kernel/arch/arm64/timer.o

# External drivers
echo "[BUILD] Compiling drivers..."
mkdir -p build/drivers/{gpu,input,network,usb,nvme,uart,video}
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/gpu/virtio_gpu.c -o build/drivers/gpu/virtio_gpu.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/input/virtio_input.c -o build/drivers/input/virtio_input.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/network/virtio_net.c -o build/drivers/network/virtio_net.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/usb/xhci.c -o build/drivers/usb/xhci.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/usb/usb_hid.c -o build/drivers/usb/usb_hid.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/usb/usb_msd.c -o build/drivers/usb/usb_msd.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/nvme/ans.c -o build/drivers/nvme/ans.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/uart/uart.c -o build/drivers/uart/uart.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/video/fb.c -o build/drivers/video/fb.o
"$CC" $CFLAGS $CROSS_TARGET -Ikernel/include -c drivers/video/ramfb.c -o build/drivers/video/ramfb.o

# Link kernel
echo "[LINK] Linking kernel..."
"$LD" -nostdlib -static -T kernel/linker.ld \
    build/kernel/arch/arm64/boot.o \
    build/kernel/arch/arm64/switch.o \
    build/kernel/arch/arm64/arch.o \
    build/kernel/arch/arm64/gic.o \
    build/kernel/arch/arm64/timer.o \
    build/kernel/core/*.o \
    build/kernel/mm/*.o \
    build/kernel/sched/*.o \
    build/kernel/fs/*.o \
    build/kernel/net/*.o \
    build/kernel/syscall/*.o \
    build/kernel/drivers/*.o \
    build/kernel/drivers/block/*.o \
    build/kernel/gui/*.o \
    build/kernel/apps/*.o \
    build/kernel/media/*.o \
    build/kernel/sandbox/*.o \
    build/kernel/sync/*.o \
    build/kernel/ipc/*.o \
    build/kernel/loader/*.o \
    build/kernel/lib/*.o \
    build/kernel/lib/partition/*.o \
    build/kernel/installer/*.o \
    build/kernel/assets/*.o \
    build/drivers/*/*.o \
    -o build/kernel/unixos.elf

echo ""
echo "========================================"
echo "✓ Kernel built successfully!"
echo "========================================"
echo ""
echo "Output: build/kernel/unixos.elf"
ls -lh build/kernel/unixos.elf
echo ""
