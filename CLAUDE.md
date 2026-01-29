# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Vib-OS is a from-scratch, Unix-like operating system with full multi-architecture support (ARM64, x86_64, x86). It features a custom kernel with a macOS-inspired GUI, TCP/IP networking, and a Virtual File System. The codebase contains 25,000+ lines of C and Assembly.

**Current Branch:** WFL (Work in progress)
**Main Branch:** main
**Version:** 2.2.0

## Build Commands

### Basic Build (ARM64 - Default)
```bash
make all          # Build everything (kernel, drivers, libc, userspace)
make kernel       # Build kernel only
make drivers      # Build device drivers
make libc         # Build C library (musl)
make userspace    # Build userspace programs
make image        # Create bootable disk image
make clean        # Remove build artifacts
```

### Multi-Architecture Build
```bash
# x86_64
make -f Makefile.multiarch ARCH=x86_64 clean
make -f Makefile.multiarch ARCH=x86_64 kernel
make -f Makefile.multiarch ARCH=x86_64 qemu

# x86 (32-bit)
make -f Makefile.multiarch ARCH=x86 clean
make -f Makefile.multiarch ARCH=x86 kernel

# ARM64
make -f Makefile.multiarch ARCH=arm64 kernel
```

### Running and Testing
```bash
make run          # Run in QEMU (text mode)
make run-gui      # Run in QEMU with GUI (recommended for development)
make run-gpu      # Run with virtio-GPU acceleration
make qemu         # Direct kernel boot (headless)
make qemu-debug   # Run with GDB server on port 1234
```

### Debugging
```bash
# Start kernel in debug mode
make qemu-debug

# In another terminal, connect with GDB
gdb-multiarch build/kernel/unixos.elf
(gdb) target remote localhost:1234
(gdb) continue
```

## Architecture

### Directory Structure

```
vib-OS/
├── kernel/              # Kernel source
│   ├── arch/           # Architecture-specific code
│   │   ├── arm64/      # ARM64: boot.S, arch.c, gic.c, timer.c, switch.S
│   │   ├── x86_64/     # x86_64: boot.S, arch.c, apic.c, pit.c, switch.S
│   │   └── x86/        # x86 32-bit support
│   ├── core/           # Core kernel (main.c, panic.c, printk.c)
│   ├── mm/             # Memory management (PMM, VMM, kmalloc, ASLR)
│   ├── sched/          # Process scheduler (sched.c, fork.c, signal.c)
│   ├── fs/             # Filesystem (VFS, RamFS, EXT4, APFS)
│   ├── net/            # Networking (TCP/IP stack, DNS, sockets)
│   ├── syscall/        # System call interface
│   ├── drivers/        # Hardware drivers (PCI, audio)
│   ├── gui/            # GUI system (window manager, desktop, terminal)
│   ├── apps/           # Built-in GUI applications
│   ├── media/          # Media decoders (JPEG, MP3) and assets
│   ├── sandbox/        # Security sandbox
│   ├── sync/           # Synchronization primitives
│   ├── ipc/            # Inter-process communication
│   ├── loader/         # ELF loader
│   └── lib/            # Kernel library functions
├── drivers/            # External drivers (network, video, input, USB, NVMe)
├── user/               # Userspace binaries
│   └── bin/doom/       # Doom port
├── scripts/            # Build and deployment scripts
├── boot/               # Boot configuration
├── Makefile            # ARM64 build (default)
└── Makefile.multiarch  # Multi-arch build system
```

### Key Subsystems

#### 1. Architecture Abstraction Layer (kernel/arch/)
- **Purpose:** Provides unified interface across ARM64, x86_64, and x86
- **Key Files:**
  - `kernel/include/arch/arch.h` - Architecture abstraction interface
  - `kernel/arch/arm64/boot.S` - ARM64 boot code and exception vectors
  - `kernel/arch/x86_64/boot.S` - x86_64 boot code
  - `kernel/arch/arm64/arch.c` - ARM64-specific initialization
  - `kernel/arch/arm64/gic.c` - ARM64 GICv3 interrupt controller
  - `kernel/arch/x86_64/apic.c` - x86_64 APIC interrupt controller
  - `kernel/arch/*/switch.S` - Context switching (architecture-specific)

#### 2. Memory Management (kernel/mm/)
- **PMM (Physical Memory Manager):** `pmm.c` - Page frame allocation
- **VMM (Virtual Memory Manager):** `vmm.c` - 4-level page tables (ARM64/x86_64)
- **Kernel Allocator:** `kmalloc.c` - Kernel heap management
- **ASLR:** `aslr.c` - Address Space Layout Randomization for security
- **Important:** Media files in kernel/media/ compile WITHOUT `-mgeneral-regs-only` to allow FP operations

#### 3. Process Scheduler (kernel/sched/)
- **Core:** `sched.c` - Preemptive multitasking with priority-based scheduling
- **Process Creation:** `fork.c` - Process/thread creation via fork() and clone()
- **Signals:** `signal.c` - UNIX signal handling
- **Key Structures:**
  - `struct task_struct` - Process control block (defined in `include/sched/sched.h`)
  - `struct cpu_context` - Saved CPU registers for context switching
  - States: RUNNING, INTERRUPTIBLE, UNINTERRUPTIBLE, STOPPED, ZOMBIE, DEAD

#### 4. Virtual Filesystem (kernel/fs/)
- **VFS Core:** `vfs.c` - Unified filesystem interface
- **RamFS:** `ramfs.c` - In-memory filesystem (default root filesystem)
- **EXT4:** `ext4.c` - Full read/write support with block/inode bitmaps
- **APFS:** `apfs.c` - Read-only support for Apple File System
- **Key Structures:**
  - `struct inode` - File metadata
  - `struct dentry` - Directory entry
  - `struct file` - Open file descriptor
  - `struct super_block` - Filesystem superblock

#### 5. Networking Stack (kernel/net/)
- **TCP/IP:** `tcp_ip.c` - Full Ethernet/ARP/IP/ICMP/UDP/TCP implementation
- **DNS:** `dns.c` - Built-in DNS resolver
- **Sockets:** `socket.c` - Berkeley sockets API
- **Driver:** `drivers/network/virtio_net.c` - Virtio-Net device driver

#### 6. System Calls (kernel/syscall/)
- **Interface:** `syscall.c` - System call dispatcher
- **ARM64:** Uses `SVC` instruction (handled in `kernel/arch/arm64/boot.S`)
- **x86_64:** Uses `INT 0x80` or `SYSCALL` instruction
- **Numbers:** Linux ARM64 compatible (defined in `include/syscall/syscall.h`)

#### 7. GUI System (kernel/gui/)
- **Window Manager:** `window.c` - Draggable/resizable windows with focus management
- **Desktop:** `desktop.c` - Desktop environment with dock and menu bar
- **Terminal:** `terminal.c` - VT100-compatible terminal emulator
- **Font Rendering:** `font.c` - Bitmap font renderer
- **Applications:** `kernel/apps/` - Built-in apps (file manager, calculator, notepad, etc.)
- **Compositor:** Double-buffered rendering for flicker-free graphics

#### 8. Media Support (kernel/media/)
- **JPEG Decoder:** `picojpeg.c` - Image viewing support
- **PNG Decoder:** `tpng.c` - PNG image support
- **MP3 Decoder:** Uses minimp3 library
- **Bootstrap Assets:** `bootstrap_*.c` - Embedded wallpaper and test images
- **Sandbox:** `kernel/sandbox/sandbox.c` - Fault-isolating media decoder sandbox

#### 9. Drivers (drivers/)
- **Virtio:** Network (virtio-net), input (keyboard/tablet)
- **Audio:** Intel HDA (kernel/drivers/audio/) - 16-bit stereo PCM playback
- **Video:** Ramfb (ARM64), VGA/VESA (x86_64)
- **Serial:** PL011 UART (ARM64), 16550 UART (x86_64)
- **USB:** Basic USB support (drivers/usb/)
- **NVMe:** NVMe storage driver (drivers/nvme/)

## Important Implementation Details

### Context Switching
- **ARM64:** Uses callee-saved registers (x19-x29, sp, pc) in `struct cpu_context`
- **x86_64:** Saves all general-purpose registers, flags, and segment selectors
- **Assembly:** Context switch implemented in `kernel/arch/*/switch.S`
- **Called by:** `schedule()` in `kernel/sched/sched.c`

### Interrupt Handling
- **ARM64:**
  - GICv3 interrupt controller (`kernel/arch/arm64/gic.c`)
  - Exception vectors in `kernel/arch/arm64/boot.S`
  - Timer: ARM Generic Timer (`kernel/arch/arm64/timer.c`)
- **x86_64:**
  - APIC interrupt controller (`kernel/arch/x86_64/apic.c`)
  - IDT setup in `kernel/arch/x86_64/arch.c`
  - Timer: PIT (`kernel/arch/x86_64/pit.c`)

### ELF Loading
- **Loader:** `kernel/loader/elf.c`
- **Execution:** `sys_execve` in `kernel/syscall/syscall.c`
- **Process:**
  1. Parse ELF headers and validate
  2. Map segments into user address space
  3. Set up user stack (argc, argv, envp)
  4. Jump to userspace via `eret` (ARM64) or `iretq` (x86_64)

### Spinlocks and Synchronization
- **Implementation:** `kernel/sync/`
- **IRQ-safe:** Spinlocks disable interrupts when acquired
- **Used by:** Network stack, filesystem, scheduler

### Toolchain
- **Compiler:** Clang/LLVM (cross-compilation)
- **Linker:** LLD
- **ARM64 Target:** `aarch64-unknown-none-elf`
- **x86_64 Target:** `x86_64-unknown-none-elf`
- **Flags:**
  - ARM64: `-mcpu=cortex-a72 -mgeneral-regs-only` (except media files)
  - x86_64: `-mcmodel=kernel -mno-red-zone -mno-sse`

## Development Guidelines

### Adding Architecture Support
1. Create new directory: `kernel/arch/<arch>/`
2. Implement required functions in `kernel/include/arch/arch.h`
3. Add boot code: `boot.S`
4. Add architecture detection to `kernel/include/arch/arch.h`
5. Update `Makefile.multiarch` with new ARCH option
6. Add linker script: `kernel/linker_<arch>.ld`

### Adding a New Filesystem
1. Implement VFS operations in `kernel/fs/<fsname>.c`
2. Register filesystem with VFS in `kernel/fs/vfs.c`
3. Create filesystem structure following pattern in `kernel/include/fs/vfs.h`
4. Implement: `mount`, `unmount`, `open`, `read`, `write`, `mkdir`, `readdir`

### Adding a New Driver
1. Create driver in `drivers/<category>/<driver>.c`
2. Use existing virtio drivers as reference
3. Register interrupt handlers via architecture-specific IRQ subsystem
4. For PCI devices, use `kernel/drivers/pci.c` functions

### Adding a GUI Application
1. Create application in `kernel/apps/<appname>.c`
2. Use window manager API from `kernel/include/gui/window.h`
3. Register application launcher in desktop dock
4. Handle mouse/keyboard events in application event loop

### Building for Real Hardware
#### Raspberry Pi 4/5 (ARM64):
```bash
make image
sudo dd if=image/unixos.img of=/dev/sdX bs=4M status=progress && sync
```

#### x86_64 PC:
```bash
./scripts/create-uefi-image.sh     # UEFI boot
./scripts/create-bios-image.sh     # Legacy BIOS boot
./scripts/create-iso.sh            # Bootable ISO
sudo dd if=vibos-uefi.img of=/dev/sdX bs=4M status=progress && sync
```

## Known Limitations and Issues

1. **Sound:** Intel HDA driver works but may have choppy audio in QEMU
2. **x86_64:** Kernel builds successfully but needs more testing on real hardware
3. **x86 32-bit:** Build infrastructure complete, testing in progress
4. **SMP:** Infrastructure initialized, but secondary CPU support not fully active
5. **USB:** Basic support present, needs driver implementation
6. **Network Settings UI:** Not fully implemented (command-line works)

## Testing and CI/CD

- **Test Script:** `scripts/run-tests.sh`
- **Automated Testing:** Designed for headless QEMU (`make qemu`)
- **Platform Testing:** Test on both ARM64 (QEMU/Pi) and x86_64 (QEMU/PC)
- **Debugging:** Use `make qemu-debug` + GDB for kernel debugging

## Performance Considerations

- **Memory:** Minimum 512MB RAM recommended, 4GB for best performance
- **CPU:** Optimized for Cortex-A72 (ARM64) and modern x86_64 CPUs
- **GUI:** Hardware framebuffer access for maximum performance
- **Network:** Zero-copy buffers in virtio-net driver for high throughput

## Security Features

- **ASLR:** Randomizes user process address space
- **NX Protection:** Non-executable pages for stack/heap
- **Spinlocks:** IRQ-safe synchronization primitives
- **Sandbox:** Media decoder sandbox isolates faults
- **Stack Protection:** `-fstack-protector-strong` in kernel build

## Useful Debugging Tips

1. **Kernel Panics:** Check `kernel/core/panic.c` for stack traces
2. **Serial Output:** Use `-serial stdio` in QEMU to capture kernel logs
3. **GDB Symbols:** Build with `-g` flag (already default)
4. **Memory Issues:** Check PMM/VMM allocation in `kernel/mm/`
5. **Scheduler Issues:** Add printk() in `kernel/sched/sched.c:schedule()`
6. **Filesystem Issues:** Enable VFS debug output in `kernel/fs/vfs.c`
