/*
 * vib-OS Block Device Interface
 *
 * Unified interface for block storage devices (virtio-block, NVMe, etc.)
 */

#ifndef BLOCK_DEV_H
#define BLOCK_DEV_H

#include "types.h"

/* Block device structure */
struct block_device {
    char name[32];              /* Device name (e.g., "vda", "nvme0n1") */
    uint64_t size_bytes;        /* Total device size in bytes */
    uint32_t block_size;        /* Block size in bytes (usually 512) */
    void *driver_data;          /* Driver-specific data */

    /* Operations */
    int (*read)(struct block_device *dev, uint64_t lba, void *buf, uint32_t count);
    int (*write)(struct block_device *dev, uint64_t lba, const void *buf, uint32_t count);

    /* Linked list */
    struct block_device *next;
};

typedef struct block_device block_device_t;

/* Initialize block device subsystem */
void block_dev_init(void);

/* Register a new block device */
int block_dev_register(block_device_t *dev);

/* Enumerate all block devices */
block_device_t *block_dev_enumerate(void);

/* Find device by name */
block_device_t *block_dev_find(const char *name);

/* Get first device */
block_device_t *block_dev_get_first(void);

/* Helper: Calculate number of blocks for byte range */
static inline uint32_t block_dev_bytes_to_blocks(block_device_t *dev, uint64_t bytes) {
    return (uint32_t)((bytes + dev->block_size - 1) / dev->block_size);
}

/* Helper: Read bytes (handles partial blocks) */
int block_dev_read_bytes(block_device_t *dev, uint64_t offset, void *buf, size_t size);

/* Helper: Write bytes (handles partial blocks) */
int block_dev_write_bytes(block_device_t *dev, uint64_t offset, const void *buf, size_t size);

#endif /* BLOCK_DEV_H */
