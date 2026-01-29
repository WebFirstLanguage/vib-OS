/*
 * vib-OS EXT4 Filesystem Creation (mkfs.ext4)
 */

#ifndef EXT4_MKFS_H
#define EXT4_MKFS_H

#include "drivers/block_dev.h"
#include "types.h"

/*
 * ext4_mkfs - Create new EXT4 filesystem
 * @dev: Block device
 * @start_lba: Starting LBA of partition
 * @num_sectors: Number of sectors in partition
 * @volume_label: Volume label (max 16 chars)
 *
 * Returns: 0 on success, negative on error
 */
int ext4_mkfs(block_device_t *dev, uint64_t start_lba, uint64_t num_sectors,
              const char *volume_label);

#endif /* EXT4_MKFS_H */
