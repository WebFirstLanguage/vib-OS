/*
 * vib-OS GPT (GUID Partition Table) Support
 *
 * Create and manipulate GPT partition tables
 */

#ifndef GPT_H
#define GPT_H

#include "drivers/block_dev.h"
#include "types.h"

/* GPT Signature */
#define GPT_SIGNATURE 0x5452415020494645ULL /* "EFI PART" */

/* GPT Revision */
#define GPT_REVISION 0x00010000 /* 1.0 */

/* GPT Header Size */
#define GPT_HEADER_SIZE 92

/* Standard Partition Type GUIDs */

/* EFI System Partition */
#define GPT_TYPE_EFI_SYSTEM                                                    \
  {                                                                            \
    0x28, 0x73, 0x2a, 0xc1, 0x1f, 0xf8, 0xd2, 0x11, 0xba, 0x4b, 0x00, 0xa0,  \
        0xc9, 0x3e, 0xc9, 0x3b                                                 \
  }

/* Linux Filesystem */
#define GPT_TYPE_LINUX_FILESYSTEM                                              \
  {                                                                            \
    0xaf, 0x3d, 0xc6, 0x0f, 0x83, 0x84, 0x72, 0x47, 0x8e, 0x79, 0x3d, 0x69,  \
        0xd8, 0x47, 0x7d, 0xe4                                                 \
  }

/* Linux Swap */
#define GPT_TYPE_LINUX_SWAP                                                    \
  {                                                                            \
    0x82, 0x65, 0x16, 0x06, 0x36, 0xd3, 0x11, 0x4d, 0xba, 0x42, 0x00, 0xa0,  \
        0xc9, 0x3e, 0xc9, 0x3b                                                 \
  }

/* GPT Header Structure */
typedef struct __attribute__((packed)) {
    uint64_t signature;         /* "EFI PART" */
    uint32_t revision;          /* 0x00010000 */
    uint32_t header_size;       /* 92 bytes */
    uint32_t header_crc32;      /* CRC32 of header (with this field set to 0) */
    uint32_t reserved;          /* Must be 0 */
    uint64_t my_lba;            /* LBA of this header */
    uint64_t alternate_lba;     /* LBA of alternate header */
    uint64_t first_usable_lba;  /* First usable LBA for partitions */
    uint64_t last_usable_lba;   /* Last usable LBA for partitions */
    uint8_t disk_guid[16];      /* Unique disk GUID */
    uint64_t partition_entry_lba; /* Starting LBA of partition entries array */
    uint32_t num_partition_entries; /* Number of partition entries (128) */
    uint32_t partition_entry_size;  /* Size of each partition entry (128) */
    uint32_t partition_array_crc32; /* CRC32 of partition entries array */
    /* Remainder of block is reserved (420 bytes) */
} gpt_header_t;

/* GPT Partition Entry Structure */
typedef struct __attribute__((packed)) {
    uint8_t type_guid[16];      /* Partition type GUID */
    uint8_t unique_guid[16];    /* Unique partition GUID */
    uint64_t first_lba;         /* First LBA of partition */
    uint64_t last_lba;          /* Last LBA of partition (inclusive) */
    uint64_t attributes;        /* Attribute flags */
    uint16_t name[36];          /* Partition name (UTF-16LE, 72 bytes) */
} gpt_entry_t;

/* GPT Context (tracks GPT state) */
typedef struct {
    block_device_t *dev;
    gpt_header_t header;
    gpt_entry_t *entries;       /* Array of 128 partition entries */
    int num_entries;
} gpt_context_t;

/* Initialize new GPT on device */
int gpt_create(block_device_t *dev, gpt_context_t *ctx);

/* Add partition to GPT */
int gpt_add_partition(gpt_context_t *ctx, int index, const char *name,
                      uint64_t start_lba, uint64_t end_lba,
                      const uint8_t type_guid[16]);

/* Write GPT to disk (primary and backup) */
int gpt_write(gpt_context_t *ctx);

/* Helper: Convert ASCII name to UTF-16LE */
void gpt_name_to_utf16(uint16_t *dest, const char *src, size_t max_chars);

/* Helper: Generate random GUID */
void gpt_generate_guid(uint8_t guid[16]);

#endif /* GPT_H */
