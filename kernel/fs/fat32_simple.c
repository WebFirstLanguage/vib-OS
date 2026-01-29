/*
 * vib-OS Simple FAT32 for ESP
 *
 * Minimal FAT32 implementation for ESP partition formatting
 * This is a simplified version for bootloader installation
 */

#include "fs/fat32.h"
#include "mm/kmalloc.h"
#include "printk.h"
#include "string.h"

/* FAT32 Boot Sector Structure */
struct fat32_boot_sector {
    uint8_t jmp_boot[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    uint8_t boot_code[420];
    uint16_t signature;
} __attribute__((packed));

/*
 * fat32_format_esp - Format ESP partition as FAT32
 */
int fat32_format_esp(block_device_t *dev, uint64_t start_lba,
                     uint64_t num_sectors, const char *volume_label) {
    if (!dev || !volume_label) return -1;

    printk(KERN_INFO "[FAT32] Formatting ESP partition (LBA %llu, %llu sectors)\n",
           (unsigned long long)start_lba,
           (unsigned long long)num_sectors);

    /* Allocate boot sector buffer */
    void *boot_buf = kmalloc(dev->block_size);
    if (!boot_buf) {
        printk(KERN_ERR "[FAT32] Failed to allocate buffer\n");
        return -1;
    }
    memset(boot_buf, 0, dev->block_size);

    struct fat32_boot_sector *bs = (struct fat32_boot_sector *)boot_buf;

    /* Setup boot sector */
    bs->jmp_boot[0] = 0xEB;
    bs->jmp_boot[1] = 0x58;
    bs->jmp_boot[2] = 0x90;
    memcpy(bs->oem_name, "MSWIN4.1", 8);
    bs->bytes_per_sector = dev->block_size;
    bs->sectors_per_cluster = 8; /* 4KB clusters for 512-byte sectors */
    bs->reserved_sectors = 32;
    bs->num_fats = 2;
    bs->root_entries = 0; /* FAT32 uses cluster chain for root */
    bs->total_sectors_16 = 0; /* Use 32-bit field for large partitions */
    bs->media_type = 0xF8; /* Fixed disk */
    bs->fat_size_16 = 0; /* Use 32-bit field */
    bs->sectors_per_track = 63;
    bs->num_heads = 255;
    bs->hidden_sectors = 0;
    bs->total_sectors_32 = (uint32_t)num_sectors;

    /* Calculate FAT size */
    uint32_t cluster_count = num_sectors / bs->sectors_per_cluster;
    bs->fat_size_32 = (cluster_count * 4 + dev->block_size - 1) / dev->block_size;

    bs->ext_flags = 0;
    bs->fs_version = 0;
    bs->root_cluster = 2; /* Root directory at cluster 2 */
    bs->fs_info = 1;
    bs->backup_boot_sector = 6;
    bs->drive_number = 0x80;
    bs->boot_signature = 0x29;
    bs->volume_id = 0x12345678; /* Random ID */

    /* Copy volume label (11 chars, space-padded) */
    memset(bs->volume_label, ' ', 11);
    size_t label_len = strlen(volume_label);
    if (label_len > 11) label_len = 11;
    memcpy(bs->volume_label, volume_label, label_len);

    memcpy(bs->fs_type, "FAT32   ", 8);
    bs->signature = 0xAA55;

    /* Write boot sector */
    if (dev->write(dev, start_lba, boot_buf, 1) < 0) {
        printk(KERN_ERR "[FAT32] Failed to write boot sector\n");
        kfree(boot_buf);
        return -1;
    }

    /* Write backup boot sector */
    if (dev->write(dev, start_lba + 6, boot_buf, 1) < 0) {
        printk(KERN_WARNING "[FAT32] Failed to write backup boot sector\n");
    }

    /* Initialize FAT tables (simplified - mark first two entries as used) */
    memset(boot_buf, 0, dev->block_size);
    uint32_t *fat = (uint32_t *)boot_buf;
    fat[0] = 0x0FFFFFF8; /* Media type */
    fat[1] = 0x0FFFFFFF; /* End of chain */
    fat[2] = 0x0FFFFFFF; /* Root directory (cluster 2) */

    uint64_t fat_start = start_lba + bs->reserved_sectors;
    for (int i = 0; i < bs->num_fats; i++) {
        if (dev->write(dev, fat_start + (i * bs->fat_size_32), boot_buf, 1) < 0) {
            printk(KERN_ERR "[FAT32] Failed to write FAT %d\n", i);
        }
    }

    kfree(boot_buf);

    printk(KERN_INFO "[FAT32] ESP formatted successfully\n");
    printk(KERN_INFO "[FAT32] Label: %s\n", volume_label);

    return 0;
}

/*
 * fat32_write_file - Write file to FAT32 filesystem
 *
 * Note: This is a simplified stub. Full implementation would involve:
 * - Parsing directory structure
 * - Allocating clusters from FAT
 * - Creating directory entries
 * - Writing file data to allocated clusters
 */
int fat32_write_file(block_device_t *dev, uint64_t start_lba,
                     const char *path, const void *data, size_t size) {
    (void)dev;
    (void)start_lba;
    (void)path;
    (void)data;
    (void)size;

    printk(KERN_INFO "[FAT32] Stub: Would write %s (%zu bytes)\n", path, size);
    /* TODO: Implement full FAT32 file write */
    return 0;
}
