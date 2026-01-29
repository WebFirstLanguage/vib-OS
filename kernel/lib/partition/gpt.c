/*
 * vib-OS GPT Partition Table Implementation
 */

#include "lib/partition/gpt.h"
#include "lib/crc32.h"
#include "mm/kmalloc.h"
#include "printk.h"
#include "string.h"
#include "arch/arch.h"

/* Protective MBR Structure */
typedef struct __attribute__((packed)) {
    uint8_t boot_code[440];
    uint32_t disk_signature;
    uint16_t reserved;
    struct {
        uint8_t status;
        uint8_t first_chs[3];
        uint8_t type;
        uint8_t last_chs[3];
        uint32_t first_lba;
        uint32_t num_sectors;
    } partitions[4];
    uint16_t signature;
} mbr_t;

/*
 * gpt_create - Initialize new GPT on device
 * @dev: Block device
 * @ctx: GPT context to initialize
 *
 * Returns: 0 on success, negative on error
 */
int gpt_create(block_device_t *dev, gpt_context_t *ctx) {
    if (!dev || !ctx) return -1;

    printk(KERN_INFO "[GPT] Creating GPT on %s\n", dev->name);

    /* Initialize context */
    memset(ctx, 0, sizeof(gpt_context_t));
    ctx->dev = dev;
    ctx->num_entries = 128;

    /* Allocate partition entries array */
    ctx->entries = kmalloc(sizeof(gpt_entry_t) * ctx->num_entries);
    if (!ctx->entries) {
        printk(KERN_ERR "[GPT] Failed to allocate partition entries\n");
        return -1;
    }
    memset(ctx->entries, 0, sizeof(gpt_entry_t) * ctx->num_entries);

    /* Calculate disk geometry */
    uint64_t total_sectors = dev->size_bytes / dev->block_size;
    uint64_t partition_array_size = (ctx->num_entries * sizeof(gpt_entry_t) + dev->block_size - 1) / dev->block_size;

    /* Setup GPT header */
    ctx->header.signature = GPT_SIGNATURE;
    ctx->header.revision = GPT_REVISION;
    ctx->header.header_size = GPT_HEADER_SIZE;
    ctx->header.header_crc32 = 0; /* Calculated later */
    ctx->header.reserved = 0;
    ctx->header.my_lba = 1; /* Primary header at LBA 1 */
    ctx->header.alternate_lba = total_sectors - 1; /* Backup at last sector */
    ctx->header.first_usable_lba = 2 + partition_array_size;
    ctx->header.last_usable_lba = total_sectors - 2 - partition_array_size;
    ctx->header.partition_entry_lba = 2; /* Entries start at LBA 2 */
    ctx->header.num_partition_entries = ctx->num_entries;
    ctx->header.partition_entry_size = sizeof(gpt_entry_t);
    ctx->header.partition_array_crc32 = 0; /* Calculated later */

    /* Generate disk GUID */
    gpt_generate_guid(ctx->header.disk_guid);

    printk(KERN_INFO "[GPT] Usable LBA range: %llu - %llu\n",
           (unsigned long long)ctx->header.first_usable_lba,
           (unsigned long long)ctx->header.last_usable_lba);

    return 0;
}

/*
 * gpt_add_partition - Add partition to GPT
 * @ctx: GPT context
 * @index: Partition index (0-127)
 * @name: Partition name (ASCII)
 * @start_lba: Starting LBA
 * @end_lba: Ending LBA (inclusive)
 * @type_guid: Partition type GUID
 *
 * Returns: 0 on success, negative on error
 */
int gpt_add_partition(gpt_context_t *ctx, int index, const char *name,
                      uint64_t start_lba, uint64_t end_lba,
                      const uint8_t type_guid[16]) {
    if (!ctx || index < 0 || index >= ctx->num_entries) return -1;

    if (start_lba < ctx->header.first_usable_lba ||
        end_lba > ctx->header.last_usable_lba) {
        printk(KERN_ERR "[GPT] Partition %d out of usable range\n", index);
        return -1;
    }

    gpt_entry_t *entry = &ctx->entries[index];

    /* Copy type GUID */
    memcpy(entry->type_guid, type_guid, 16);

    /* Generate unique partition GUID */
    gpt_generate_guid(entry->unique_guid);

    /* Set LBA range */
    entry->first_lba = start_lba;
    entry->last_lba = end_lba;
    entry->attributes = 0;

    /* Convert name to UTF-16LE */
    gpt_name_to_utf16(entry->name, name, 36);

    printk(KERN_INFO "[GPT] Added partition %d: %s (LBA %llu - %llu)\n",
           index, name,
           (unsigned long long)start_lba,
           (unsigned long long)end_lba);

    return 0;
}

/*
 * gpt_write - Write GPT to disk
 * @ctx: GPT context
 *
 * Returns: 0 on success, negative on error
 */
int gpt_write(gpt_context_t *ctx) {
    if (!ctx || !ctx->dev) return -1;

    block_device_t *dev = ctx->dev;
    printk(KERN_INFO "[GPT] Writing GPT to %s\n", dev->name);

    /* Allocate buffers */
    uint8_t *mbr_buf = kmalloc(dev->block_size);
    uint8_t *header_buf = kmalloc(dev->block_size);
    uint8_t *entries_buf = kmalloc(ctx->num_entries * sizeof(gpt_entry_t));

    if (!mbr_buf || !header_buf || !entries_buf) {
        printk(KERN_ERR "[GPT] Failed to allocate write buffers\n");
        kfree(mbr_buf);
        kfree(header_buf);
        kfree(entries_buf);
        return -1;
    }

    memset(mbr_buf, 0, dev->block_size);
    memset(header_buf, 0, dev->block_size);
    memcpy(entries_buf, ctx->entries, ctx->num_entries * sizeof(gpt_entry_t));

    /* Step 1: Write Protective MBR */
    mbr_t *mbr = (mbr_t *)mbr_buf;
    mbr->signature = 0xAA55;
    mbr->partitions[0].status = 0x00;
    mbr->partitions[0].type = 0xEE; /* GPT Protective */
    mbr->partitions[0].first_lba = 1;
    mbr->partitions[0].num_sectors = (uint32_t)((dev->size_bytes / dev->block_size) - 1);
    if (mbr->partitions[0].num_sectors == 0) mbr->partitions[0].num_sectors = 0xFFFFFFFF;

    if (dev->write(dev, 0, mbr_buf, 1) < 0) {
        printk(KERN_ERR "[GPT] Failed to write protective MBR\n");
        goto error;
    }

    /* Step 2: Calculate partition array CRC32 */
    ctx->header.partition_array_crc32 = crc32_compute(entries_buf,
                                                       ctx->num_entries * sizeof(gpt_entry_t));

    /* Step 3: Calculate header CRC32 */
    ctx->header.header_crc32 = 0;
    ctx->header.header_crc32 = crc32_compute(&ctx->header, ctx->header.header_size);

    /* Step 4: Write primary GPT header */
    memcpy(header_buf, &ctx->header, sizeof(gpt_header_t));
    if (dev->write(dev, 1, header_buf, 1) < 0) {
        printk(KERN_ERR "[GPT] Failed to write primary GPT header\n");
        goto error;
    }

    /* Step 5: Write partition entries array */
    uint32_t entry_blocks = (ctx->num_entries * sizeof(gpt_entry_t) + dev->block_size - 1) / dev->block_size;
    if (dev->write(dev, 2, entries_buf, entry_blocks) < 0) {
        printk(KERN_ERR "[GPT] Failed to write partition entries\n");
        goto error;
    }

    /* Step 6: Write backup GPT */
    /* Backup header has different LBA values */
    gpt_header_t backup_header = ctx->header;
    backup_header.my_lba = ctx->header.alternate_lba;
    backup_header.alternate_lba = ctx->header.my_lba;
    backup_header.partition_entry_lba = ctx->header.alternate_lba - entry_blocks;

    /* Recalculate backup header CRC */
    backup_header.header_crc32 = 0;
    backup_header.header_crc32 = crc32_compute(&backup_header, backup_header.header_size);

    /* Write backup partition entries */
    if (dev->write(dev, backup_header.partition_entry_lba, entries_buf, entry_blocks) < 0) {
        printk(KERN_ERR "[GPT] Failed to write backup partition entries\n");
        goto error;
    }

    /* Write backup header */
    memset(header_buf, 0, dev->block_size);
    memcpy(header_buf, &backup_header, sizeof(gpt_header_t));
    if (dev->write(dev, backup_header.my_lba, header_buf, 1) < 0) {
        printk(KERN_ERR "[GPT] Failed to write backup GPT header\n");
        goto error;
    }

    printk(KERN_INFO "[GPT] Successfully wrote GPT to disk\n");

    kfree(mbr_buf);
    kfree(header_buf);
    kfree(entries_buf);
    return 0;

error:
    kfree(mbr_buf);
    kfree(header_buf);
    kfree(entries_buf);
    return -1;
}

/*
 * gpt_name_to_utf16 - Convert ASCII name to UTF-16LE
 * @dest: Destination UTF-16LE buffer
 * @src: Source ASCII string
 * @max_chars: Maximum characters to convert (36 for GPT)
 */
void gpt_name_to_utf16(uint16_t *dest, const char *src, size_t max_chars) {
    size_t i = 0;
    while (i < max_chars && src[i]) {
        dest[i] = (uint16_t)src[i]; /* Simple ASCII to UTF-16LE */
        i++;
    }
    while (i < max_chars) {
        dest[i++] = 0; /* Zero-fill remaining */
    }
}

/*
 * gpt_generate_guid - Generate random GUID
 * @guid: 16-byte GUID buffer
 *
 * Note: This is a simple GUID generator using timer-based seed.
 * For production, use a better random source.
 */
void gpt_generate_guid(uint8_t guid[16]) {
    static uint64_t counter = 0;
    uint64_t time = arch_timer_get_ms();

    /* Mix timer and counter to generate pseudo-random GUID */
    uint64_t seed = time ^ (counter++ << 32);

    for (int i = 0; i < 16; i++) {
        seed = seed * 1103515245 + 12345; /* Simple LCG */
        guid[i] = (uint8_t)(seed >> 32);
    }

    /* Set GUID version 4 (random) bits */
    guid[6] = (guid[6] & 0x0F) | 0x40; /* Version 4 */
    guid[8] = (guid[8] & 0x3F) | 0x80; /* Variant 10 */
}
