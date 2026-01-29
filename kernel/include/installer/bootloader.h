/*
 * vib-OS Bootloader Installation
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "drivers/block_dev.h"
#include "types.h"

/*
 * install_bootloader - Install bootloader to ESP
 * @dev: Block device
 * @esp_start_lba: Starting LBA of ESP partition
 * @root_partition_num: Root partition number (e.g., 2 for /dev/vda2)
 *
 * Returns: 0 on success, negative on error
 */
int install_bootloader(block_device_t *dev, uint64_t esp_start_lba,
                       uint32_t root_partition_num);

#endif /* BOOTLOADER_H */
