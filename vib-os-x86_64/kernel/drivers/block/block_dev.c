/*
 * vib-OS Block Device Subsystem
 */

#include "drivers/block_dev.h"
#include "mm/kmalloc.h"
#include "printk.h"
#include "string.h"
#include "types.h"

/* Global list of block devices */
static block_device_t *block_devices = NULL;

/*
 * block_dev_init - Initialize block device subsystem
 */
void block_dev_init(void) {
    printk(KERN_INFO "[BLOCK] Initializing block device subsystem\n");
    block_devices = NULL;
}

/*
 * block_dev_register - Register a new block device
 * @dev: Block device to register
 *
 * Returns: 0 on success, negative on error
 */
int block_dev_register(block_device_t *dev) {
    if (!dev || !dev->name[0]) {
        printk(KERN_ERR "[BLOCK] Invalid device\n");
        return -1;
    }

    if (!dev->read || !dev->write) {
        printk(KERN_ERR "[BLOCK] Device %s missing read/write ops\n", dev->name);
        return -1;
    }

    /* Add to linked list */
    dev->next = block_devices;
    block_devices = dev;

    printk(KERN_INFO "[BLOCK] Registered device %s (%llu MB, %u byte blocks)\n",
           dev->name,
           dev->size_bytes / (1024 * 1024),
           dev->block_size);

    return 0;
}

/*
 * block_dev_enumerate - Get list of all block devices
 * Returns: Pointer to first device in linked list
 */
block_device_t *block_dev_enumerate(void) {
    return block_devices;
}

/*
 * block_dev_find - Find device by name
 * @name: Device name (e.g., "vda", "nvme0n1")
 * Returns: Pointer to device or NULL if not found
 */
block_device_t *block_dev_find(const char *name) {
    block_device_t *dev = block_devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
        dev = dev->next;
    }
    return NULL;
}

/*
 * block_dev_get_first - Get first registered device
 * Returns: Pointer to first device or NULL if none
 */
block_device_t *block_dev_get_first(void) {
    return block_devices;
}

/*
 * block_dev_read_bytes - Read bytes from device (handles partial blocks)
 * @dev: Block device
 * @offset: Byte offset to read from
 * @buf: Buffer to read into
 * @size: Number of bytes to read
 *
 * Returns: 0 on success, negative on error
 */
int block_dev_read_bytes(block_device_t *dev, uint64_t offset, void *buf, size_t size) {
    if (!dev || !buf || size == 0) return -1;

    uint64_t start_lba = offset / dev->block_size;
    uint32_t start_offset = offset % dev->block_size;
    uint32_t num_blocks = (start_offset + size + dev->block_size - 1) / dev->block_size;

    /* Allocate temporary buffer for full blocks */
    void *temp_buf = kmalloc(num_blocks * dev->block_size);
    if (!temp_buf) {
        printk(KERN_ERR "[BLOCK] Failed to allocate read buffer\n");
        return -1;
    }

    /* Read full blocks */
    int ret = dev->read(dev, start_lba, temp_buf, num_blocks);
    if (ret < 0) {
        kfree(temp_buf);
        return ret;
    }

    /* Copy requested bytes */
    memcpy(buf, (uint8_t *)temp_buf + start_offset, size);
    kfree(temp_buf);

    return 0;
}

/*
 * block_dev_write_bytes - Write bytes to device (handles partial blocks)
 * @dev: Block device
 * @offset: Byte offset to write to
 * @buf: Buffer to write from
 * @size: Number of bytes to write
 *
 * Returns: 0 on success, negative on error
 */
int block_dev_write_bytes(block_device_t *dev, uint64_t offset, const void *buf, size_t size) {
    if (!dev || !buf || size == 0) return -1;

    uint64_t start_lba = offset / dev->block_size;
    uint32_t start_offset = offset % dev->block_size;
    uint32_t num_blocks = (start_offset + size + dev->block_size - 1) / dev->block_size;

    /* Allocate temporary buffer for full blocks */
    void *temp_buf = kmalloc(num_blocks * dev->block_size);
    if (!temp_buf) {
        printk(KERN_ERR "[BLOCK] Failed to allocate write buffer\n");
        return -1;
    }

    /* Read-modify-write for partial blocks */
    if (start_offset != 0 || (size % dev->block_size) != 0) {
        /* Need to preserve existing data in partial blocks */
        int ret = dev->read(dev, start_lba, temp_buf, num_blocks);
        if (ret < 0) {
            kfree(temp_buf);
            return ret;
        }
    }

    /* Copy data to write */
    memcpy((uint8_t *)temp_buf + start_offset, buf, size);

    /* Write full blocks */
    int ret = dev->write(dev, start_lba, temp_buf, num_blocks);
    kfree(temp_buf);

    return ret;
}
