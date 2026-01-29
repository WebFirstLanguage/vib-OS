/*
 * vib-OS Simple FAT32 Support for ESP
 *
 * Minimal FAT32 implementation for bootloader installation
 */

#ifndef FAT32_H
#define FAT32_H

#include "drivers/block_dev.h"
#include "types.h"

/*
 * fat32_format_esp - Format ESP partition as FAT32
 * @dev: Block device
 * @start_lba: Starting LBA of partition
 * @num_sectors: Number of sectors in partition
 * @volume_label: Volume label (max 11 chars)
 *
 * Returns: 0 on success, negative on error
 */
int fat32_format_esp(block_device_t *dev, uint64_t start_lba,
                     uint64_t num_sectors, const char *volume_label);

/*
 * fat32_write_file - Write file to FAT32 filesystem
 * @dev: Block device
 * @start_lba: Starting LBA of partition
 * @path: File path (e.g., "/EFI/BOOT/BOOTAA64.EFI")
 * @data: File data
 * @size: File size
 *
 * Returns: 0 on success, negative on error
 */
int fat32_write_file(block_device_t *dev, uint64_t start_lba,
                     const char *path, const void *data, size_t size);

#endif /* FAT32_H */
