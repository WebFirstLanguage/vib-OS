/*
 * vib-OS Virtio Block Device Driver
 *
 * Provides block device interface for QEMU virtio-blk devices
 */

#include "drivers/block_dev.h"
#include "mm/kmalloc.h"
#include "mm/vmm.h"
#include "printk.h"
#include "string.h"
#include "types.h"

/* Virtio MMIO Base Address (QEMU ARM64 standard) */
#define VIRTIO_MMIO_BASE 0x0a000000UL
#define VIRTIO_MMIO_SIZE 0x200UL
#define VIRTIO_BLK_DEVICE_ID 2

/* Virtio MMIO Register Offsets */
#define VIRTIO_MMIO_MAGIC 0x000
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_READY 0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW 0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0x0a4
#define VIRTIO_MMIO_CONFIG 0x100

/* Virtio Status Bits */
#define VIRTIO_STATUS_ACK 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8

/* Block Device Configuration */
#define VIRTIO_BLK_F_RO (1 << 5)
#define VIRTIO_BLK_F_SIZE_MAX (1 << 1)
#define VIRTIO_BLK_F_SEG_MAX (1 << 2)

/* Block Request Types */
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

/* Virtqueue Descriptor */
typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} virtq_desc_t;

/* Virtqueue Available Ring */
typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[128]; /* Queue size */
} virtq_avail_t;

/* Virtqueue Used Element */
typedef struct {
    uint32_t id;
    uint32_t len;
} virtq_used_elem_t;

/* Virtqueue Used Ring */
typedef struct {
    uint16_t flags;
    uint16_t idx;
    virtq_used_elem_t ring[128]; /* Queue size */
} virtq_used_t;

#define DESC_F_NEXT 1
#define DESC_F_WRITE 2

/* Block Request Header */
struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} __attribute__((packed));

/* Driver State */
static volatile uint32_t *virtio_regs = NULL;
static virtq_desc_t *desc_table = NULL;
static virtq_avail_t *avail_ring = NULL;
static virtq_used_t *used_ring = NULL;
static uint16_t last_used_idx = 0;
static bool initialized = false;
static block_device_t virtio_blk_dev;

/* MMIO Helper Functions */
static inline uint32_t virtio_read32(uint32_t offset) {
    return virtio_regs[offset / 4];
}

static inline void virtio_write32(uint32_t offset, uint32_t val) {
    virtio_regs[offset / 4] = val;
}

/*
 * virtio_blk_rw - Perform read/write operation
 * @sector: Starting sector (512-byte units)
 * @count: Number of sectors
 * @buffer: Data buffer
 * @write: True for write, false for read
 *
 * Returns: 0 on success, negative on error
 */
static int virtio_blk_rw(uint64_t sector, uint32_t count, void *buffer,
                         bool write) {
    if (!initialized || !buffer) return -1;

    /* Allocate request structures */
    struct virtio_blk_req *req = kmalloc(sizeof(struct virtio_blk_req));
    uint8_t *status = kmalloc(1);
    if (!req || !status) {
        kfree(req);
        kfree(status);
        return -1;
    }

    /* Setup request */
    req->type = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    req->reserved = 0;
    req->sector = sector;
    *status = 0xFF;

    /* Get descriptor index */
    uint16_t desc_idx = 0;

    /* Descriptor 0: Request header (device-readable) */
    desc_table[desc_idx].addr = (uint64_t)req;
    desc_table[desc_idx].len = sizeof(struct virtio_blk_req);
    desc_table[desc_idx].flags = DESC_F_NEXT;
    desc_table[desc_idx].next = desc_idx + 1;

    /* Descriptor 1: Data buffer */
    desc_table[desc_idx + 1].addr = (uint64_t)buffer;
    desc_table[desc_idx + 1].len = count * 512;
    desc_table[desc_idx + 1].flags =
        DESC_F_NEXT | (write ? 0 : DESC_F_WRITE);
    desc_table[desc_idx + 1].next = desc_idx + 2;

    /* Descriptor 2: Status byte (device-writable) */
    desc_table[desc_idx + 2].addr = (uint64_t)status;
    desc_table[desc_idx + 2].len = 1;
    desc_table[desc_idx + 2].flags = DESC_F_WRITE;
    desc_table[desc_idx + 2].next = 0;

    /* Add to available ring */
    uint16_t avail_idx = avail_ring->idx;
    avail_ring->ring[avail_idx % 128] = desc_idx;
    __sync_synchronize(); /* Memory barrier */
    avail_ring->idx = avail_idx + 1;

    /* Notify device */
    virtio_write32(VIRTIO_MMIO_QUEUE_NOTIFY, 0);

    /* Wait for completion (simple polling) */
    int timeout = 10000;
    while (used_ring->idx == last_used_idx && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        printk(KERN_ERR "[VIRTIO-BLK] Request timeout\n");
        kfree(req);
        kfree(status);
        return -1;
    }

    last_used_idx = used_ring->idx;

    /* Check status */
    int ret = (*status == 0) ? 0 : -1;

    kfree(req);
    kfree(status);
    return ret;
}

/*
 * Block device operation wrappers
 */
static int virtio_blk_read(struct block_device *dev, uint64_t lba, void *buf,
                           uint32_t count) {
    (void)dev;
    return virtio_blk_rw(lba, count, buf, false);
}

static int virtio_blk_write(struct block_device *dev, uint64_t lba,
                            const void *buf, uint32_t count) {
    (void)dev;
    return virtio_blk_rw(lba, count, (void *)buf, true);
}

/*
 * virtio_block_init - Initialize virtio-block driver
 *
 * Scans MMIO region for virtio-block devices and registers them
 * Returns: 0 on success, negative on error
 */
int virtio_block_init(void) {
    printk(KERN_INFO "[VIRTIO-BLK] Initializing virtio-block driver\n");

    /* Scan for virtio-block devices */
    for (int i = 0; i < 8; i++) {
        uint64_t base = VIRTIO_MMIO_BASE + (i * VIRTIO_MMIO_SIZE);

        /* Map MMIO region */
        vmm_map_range(base, base, VIRTIO_MMIO_SIZE, VM_DEVICE);
        virtio_regs = (volatile uint32_t *)base;

        /* Check magic value */
        uint32_t magic = virtio_read32(VIRTIO_MMIO_MAGIC);
        if (magic != 0x74726976) { /* "virt" */
            continue;
        }

        /* Check device ID */
        uint32_t device_id = virtio_read32(VIRTIO_MMIO_DEVICE_ID);
        if (device_id != VIRTIO_BLK_DEVICE_ID) {
            continue;
        }

        printk(KERN_INFO "[VIRTIO-BLK] Found virtio-block at 0x%llx\n",
               (unsigned long long)base);

        /* Reset device */
        virtio_write32(VIRTIO_MMIO_STATUS, 0);

        /* Acknowledge device */
        virtio_write32(VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACK);

        /* Driver loaded */
        virtio_write32(VIRTIO_MMIO_STATUS,
                       VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER);

        /* Read features */
        uint32_t features = virtio_read32(VIRTIO_MMIO_DEVICE_FEATURES);
        printk(KERN_INFO "[VIRTIO-BLK] Device features: 0x%x\n", features);

        /* Accept features */
        virtio_write32(VIRTIO_MMIO_DRIVER_FEATURES, 0);
        virtio_write32(VIRTIO_MMIO_STATUS,
                       VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER |
                           VIRTIO_STATUS_FEATURES_OK);

        /* Setup virtqueue */
        virtio_write32(VIRTIO_MMIO_QUEUE_SEL, 0); /* Select queue 0 */
        uint32_t queue_size = virtio_read32(VIRTIO_MMIO_QUEUE_NUM_MAX);
        printk(KERN_INFO "[VIRTIO-BLK] Queue size: %u\n", queue_size);

        if (queue_size > 128) queue_size = 128;
        virtio_write32(VIRTIO_MMIO_QUEUE_NUM, queue_size);

        /* Allocate queue structures */
        desc_table = kmalloc(sizeof(virtq_desc_t) * queue_size);
        avail_ring = kmalloc(sizeof(virtq_avail_t));
        used_ring = kmalloc(sizeof(virtq_used_t));

        if (!desc_table || !avail_ring || !used_ring) {
            printk(KERN_ERR "[VIRTIO-BLK] Failed to allocate queues\n");
            return -1;
        }

        memset(desc_table, 0, sizeof(virtq_desc_t) * queue_size);
        memset(avail_ring, 0, sizeof(virtq_avail_t));
        memset(used_ring, 0, sizeof(virtq_used_t));

        /* Set queue addresses */
        uint64_t desc_addr = (uint64_t)desc_table;
        uint64_t avail_addr = (uint64_t)avail_ring;
        uint64_t used_addr = (uint64_t)used_ring;

        virtio_write32(VIRTIO_MMIO_QUEUE_DESC_LOW, (uint32_t)desc_addr);
        virtio_write32(VIRTIO_MMIO_QUEUE_DESC_HIGH, (uint32_t)(desc_addr >> 32));
        virtio_write32(VIRTIO_MMIO_QUEUE_AVAIL_LOW, (uint32_t)avail_addr);
        virtio_write32(VIRTIO_MMIO_QUEUE_AVAIL_HIGH, (uint32_t)(avail_addr >> 32));
        virtio_write32(VIRTIO_MMIO_QUEUE_USED_LOW, (uint32_t)used_addr);
        virtio_write32(VIRTIO_MMIO_QUEUE_USED_HIGH, (uint32_t)(used_addr >> 32));

        /* Enable queue */
        virtio_write32(VIRTIO_MMIO_QUEUE_READY, 1);

        /* Driver OK */
        virtio_write32(VIRTIO_MMIO_STATUS,
                       VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER |
                           VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);

        /* Read capacity from config space */
        uint64_t capacity = *(volatile uint64_t *)((uint8_t *)virtio_regs + VIRTIO_MMIO_CONFIG);

        printk(KERN_INFO "[VIRTIO-BLK] Capacity: %llu sectors (%llu MB)\n",
               (unsigned long long)capacity,
               (unsigned long long)(capacity / 2048));

        /* Register block device */
        strncpy(virtio_blk_dev.name, "vda", sizeof(virtio_blk_dev.name));
        virtio_blk_dev.size_bytes = capacity * 512;
        virtio_blk_dev.block_size = 512;
        virtio_blk_dev.read = virtio_blk_read;
        virtio_blk_dev.write = virtio_blk_write;
        virtio_blk_dev.driver_data = NULL;
        virtio_blk_dev.next = NULL;

        block_dev_register(&virtio_blk_dev);

        initialized = true;
        printk(KERN_INFO "[VIRTIO-BLK] Registered as /dev/vda\n");
        return 0;
    }

    printk(KERN_INFO "[VIRTIO-BLK] No virtio-block devices found\n");
    return -1;
}
