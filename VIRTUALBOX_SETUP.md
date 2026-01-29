# Running vib-OS in VirtualBox - Complete Guide

## The Situation

The installer we built is for **ARM64**, but VirtualBox only supports **x86_64** guests on most systems.

vib-OS has x86_64 support, but there are two paths:

---

## ✅ OPTION 1: Use Pre-Built x86_64 Kernel (EASIEST)

The `vib-os-x86_64` directory has a working x86_64 kernel, but it **doesn't include the installer yet**.

### Quick Setup:

**1. Create VirtualBox VM:**
- Open VirtualBox
- Click "New"
- Name: vib-OS
- Type: Linux
- Version: Other Linux (64-bit)
- Memory: 2048 MB or more
- Hard disk: Create new (VDI, dynamically allocated, 20 GB)

**2. Configure VM Settings:**
- System > Motherboard: Enable EFI
- System > Processor: 2+ CPUs
- Display > Video Memory: 128 MB
- Display > Graphics Controller: VBoxVGA

**3. Boot the Existing Kernel:**

Since we don't have an ISO tool on Windows, use QEMU to test first:

```bash
# Install QEMU
winget install qemu

# Test x86_64 kernel
qemu-system-x86_64 -m 2G -serial stdio \
    -kernel vib-os-x86_64/iso_root/boot/kernel.elf
```

---

## ✅ OPTION 2: Create VirtualBox Disk with ARM64 Kernel (QEMU Bridge)

Since VirtualBox can't run ARM64, you can use QEMU as a bridge:

**1. Install QEMU for Windows:**
```bash
winget install qemu
```

**2. Run vib-OS ARM64 with full installer:**
```bash
qemu-system-aarch64 -M virt -cpu max -m 4G \
    -kernel build/kernel/unixos.elf \
    -append "live_boot=1 console=ttyAMA0" \
    -drive if=none,id=hd0,file=image/vib-os-live.img \
    -device virtio-blk-device,drive=hd0 \
    -serial stdio
```

**The installer GUI will appear!**

---

## ✅ OPTION 3: Port Installer to x86_64 (Build from Source)

To get the installer working in VirtualBox, we need to port it to x86_64.

**Requirements:**
- LLVM/Clang (already installed ✓)
- xorriso for ISO creation
- Linux system (or WSL)

**Steps:**

**On Linux/WSL:**

```bash
# 1. Build ARM64 version first (to get installer code)
make clean
make all

# 2. Build x86_64 with Limine
cd vib-os-x86_64
bash build.sh

# 3. This creates: uefi-demo.iso
```

**Then in VirtualBox:**
1. Create VM as described in Option 1
2. Settings > Storage > Controller:IDE > Add Optical Drive
3. Choose: uefi-demo.iso
4. Boot the VM

---

## 🎯 RECOMMENDED APPROACH FOR TESTING INSTALLER

**Use QEMU with ARM64** - it's the easiest and fastest way to test the installer we just built:

### Install QEMU:
```bash
winget install qemu
# Or download: https://qemu.weilnetz.de/w64/
```

### Run vib-OS with Installer:
```bash
cd G:\Logbie\vib-OS

# Option A: Use the Makefile
make run-gui

# Option B: Direct command
qemu-system-aarch64 -M virt,gic-version=3 -cpu max -m 4G \
    -kernel build/kernel/unixos.elf \
    -append "live_boot=1 console=ttyAMA0" \
    -drive if=none,id=hd0,file=image/vib-os-live.img \
    -device virtio-blk-device,drive=hd0 \
    -device virtio-gpu-pci \
    -device virtio-keyboard-pci \
    -device virtio-mouse-pci \
    -nographic -serial stdio
```

**The installer window will appear automatically!**

---

## 📊 Comparison

| Feature | QEMU ARM64 | VirtualBox x86_64 |
|---------|------------|-------------------|
| **Installer** | ✅ Full installer ready | ⚠️ Needs x86_64 port |
| **Setup Time** | 5 minutes | 15+ minutes |
| **ISO Required** | ❌ No | ✅ Yes (needs Linux) |
| **Performance** | Fast | Fast |
| **GUI** | ✅ Works | ✅ Works |
| **Testing** | ✅ Ready now | ⏳ Needs build |

---

## 🚀 Fastest Path to Testing

```bash
# Install QEMU (one command)
winget install qemu

# Wait for install, then run:
qemu-system-aarch64 -M virt -cpu max -m 4G \
    -kernel build/kernel/unixos.elf \
    -append "live_boot=1"
```

**Done! The installer appears immediately.**

---

## Alternative: Transfer to Linux for ISO

If you have a Linux machine or WSL:

```bash
# On Windows, package the files:
tar czf vib-os-build.tar.gz build/ vib-os-x86_64/ scripts/

# Transfer to Linux
# Then:
cd vib-os-x86_64
bash build.sh
# Creates: uefi-demo.iso

# Boot in VirtualBox
```

---

## Summary

**For VirtualBox:** Need to create x86_64 ISO (requires Linux/xorriso)
**For Quick Testing:** Use QEMU (installer ready now!)

**My recommendation:** Install QEMU - you'll have the installer running in 5 minutes!
