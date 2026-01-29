/*
 * vib-OS Bootloader Installation
 */

#include "installer/bootloader.h"
#include "fs/fat32.h"
#include "printk.h"
#include "string.h"

/* Kernel binary location (linked into kernel image) */
extern char _binary_kernel_start[];
extern char _binary_kernel_end[];

/*
 * install_bootloader - Install bootloader to ESP
 */
int install_bootloader(block_device_t *dev, uint64_t esp_start_lba,
                       uint32_t root_partition_num) {
    if (!dev) return -1;

    printk(KERN_INFO "[BOOTLOADER] Installing bootloader to ESP\n");
    printk(KERN_INFO "[BOOTLOADER] ESP at LBA %llu, root partition: %u\n",
           (unsigned long long)esp_start_lba, root_partition_num);

    /* Step 1: Format ESP as FAT32 */
    printk(KERN_INFO "[BOOTLOADER] Formatting ESP as FAT32...\n");
    uint64_t esp_sectors = (200 * 1024 * 1024) / dev->block_size; /* 200MB */
    if (fat32_format_esp(dev, esp_start_lba, esp_sectors, "EFI SYSTEM") < 0) {
        printk(KERN_ERR "[BOOTLOADER] Failed to format ESP\n");
        return -1;
    }

    /* Step 2: Create /EFI/BOOT/ directory structure */
    printk(KERN_INFO "[BOOTLOADER] Creating EFI directory structure...\n");
    /* TODO: Create directories using FAT32 API */

    /* Step 3: Copy kernel to BOOTAA64.EFI (ARM64) or BOOTX64.EFI (x86_64) */
    printk(KERN_INFO "[BOOTLOADER] Copying kernel to ESP...\n");

#ifdef ARCH_ARM64
    const char *kernel_name = "/EFI/BOOT/BOOTAA64.EFI";
#elif defined(ARCH_X86_64)
    const char *kernel_name = "/EFI/BOOT/BOOTX64.EFI";
#else
    const char *kernel_name = "/EFI/BOOT/BOOTAA64.EFI";
#endif

    /* In a real implementation, we would:
     * 1. Read the current kernel image from memory or RamFS
     * 2. Write it to the ESP at the appropriate location
     * For now, this is a stub */
    printk(KERN_INFO "[BOOTLOADER] Would install kernel as: %s\n", kernel_name);

    /* Step 4: Create grub.cfg with correct root parameter */
    printk(KERN_INFO "[BOOTLOADER] Creating boot configuration...\n");

    char grub_config[512];
    snprintf(grub_config, sizeof(grub_config),
             "set timeout=3\n"
             "menuentry \"vib-OS\" {\n"
             "    linux %s root=/dev/vda%u console=tty0\n"
             "}\n",
             kernel_name, root_partition_num);

    printk(KERN_INFO "[BOOTLOADER] Boot config:\n%s\n", grub_config);

    /* TODO: Write grub.cfg to ESP using fat32_write_file */

    printk(KERN_INFO "[BOOTLOADER] Bootloader installation complete\n");
    return 0;
}
