/*
 * vib-OS EXT4 Filesystem Creation
 *
 * Creates a new EXT4 filesystem on a block device partition
 */

#include "fs/ext4_mkfs.h"
#include "lib/crc32.h"
#include "mm/kmalloc.h"
#include "printk.h"
#include "string.h"
#include "arch/arch.h"

/* EXT4 Constants */
#define EXT4_SUPER_MAGIC 0xEF53
#define EXT4_BLOCK_SIZE 4096
#define EXT4_INODE_SIZE 256
#define EXT4_INODES_PER_GROUP 8192
#define EXT4_BLOCKS_PER_GROUP 32768
#define EXT4_RESERVED_INODES 10
#define EXT4_ROOT_INO 2
#define EXT4_GOOD_OLD_FIRST_INO 11

/* Feature flags */
#define EXT4_FEATURE_INCOMPAT_FILETYPE 0x0002
#define EXT4_FEATURE_INCOMPAT_EXTENTS 0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT 0x0080
#define EXT4_FEATURE_INCOMPAT_FLEX_BG 0x0200

#define EXT4_FEATURE_COMPAT_RESIZE_INODE 0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX 0x0020

#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE 0x0002
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE 0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM 0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK 0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE 0x0040

/* Inode modes */
#define EXT4_S_IFDIR 0x4000
#define EXT4_S_IFREG 0x8000

/* Simplified superblock structure */
struct ext4_super {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count_lo;
    uint32_t s_r_blocks_count_lo;
    uint32_t s_free_blocks_count_lo;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_cluster_size;
    uint32_t s_blocks_per_group;
    uint32_t s_clusters_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t s_reserved_gdt_blocks;
    uint8_t s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t s_jnl_backup_type;
    uint16_t s_desc_size;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;
    uint32_t s_mkfs_time;
    uint32_t s_jnl_blocks[17];
    uint32_t s_blocks_count_hi;
    uint32_t s_r_blocks_count_hi;
    uint32_t s_free_blocks_count_hi;
    uint16_t s_min_extra_isize;
    uint16_t s_want_extra_isize;
    uint32_t s_flags;
    uint8_t s_padding[404];
} __attribute__((packed));

/* Group descriptor structure */
struct ext4_group_desc {
    uint32_t bg_block_bitmap_lo;
    uint32_t bg_inode_bitmap_lo;
    uint32_t bg_inode_table_lo;
    uint16_t bg_free_blocks_count_lo;
    uint16_t bg_free_inodes_count_lo;
    uint16_t bg_used_dirs_count_lo;
    uint16_t bg_flags;
    uint32_t bg_exclude_bitmap_lo;
    uint16_t bg_block_bitmap_csum_lo;
    uint16_t bg_inode_bitmap_csum_lo;
    uint16_t bg_itable_unused_lo;
    uint16_t bg_checksum;
    uint32_t bg_block_bitmap_hi;
    uint32_t bg_inode_bitmap_hi;
    uint32_t bg_inode_table_hi;
    uint16_t bg_free_blocks_count_hi;
    uint16_t bg_free_inodes_count_hi;
    uint16_t bg_used_dirs_count_hi;
    uint16_t bg_itable_unused_hi;
    uint32_t bg_exclude_bitmap_hi;
    uint16_t bg_block_bitmap_csum_hi;
    uint16_t bg_inode_bitmap_csum_hi;
    uint32_t bg_reserved;
} __attribute__((packed));

/* Inode structure */
struct ext4_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size_lo;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks_lo;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl_lo;
    uint32_t i_size_hi;
    uint8_t i_padding[96];
} __attribute__((packed));

/* Directory entry */
struct ext4_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[255];
} __attribute__((packed));

/*
 * Helper: Write block to partition
 */
static int write_partition_block(block_device_t *dev, uint64_t start_lba,
                                  uint64_t block_num, void *data) {
    uint64_t lba = start_lba + (block_num * EXT4_BLOCK_SIZE) / dev->block_size;
    uint32_t count = EXT4_BLOCK_SIZE / dev->block_size;
    return dev->write(dev, lba, data, count);
}

/*
 * ext4_mkfs - Create EXT4 filesystem
 */
int ext4_mkfs(block_device_t *dev, uint64_t start_lba, uint64_t num_sectors,
              const char *volume_label) {
    if (!dev || !volume_label) return -1;

    printk(KERN_INFO "[EXT4] Creating filesystem on %s partition (LBA %llu, %llu sectors)\n",
           dev->name, (unsigned long long)start_lba, (unsigned long long)num_sectors);

    /* Calculate filesystem parameters */
    uint64_t partition_size = num_sectors * dev->block_size;
    uint64_t num_blocks = partition_size / EXT4_BLOCK_SIZE;
    uint32_t num_groups = (num_blocks + EXT4_BLOCKS_PER_GROUP - 1) / EXT4_BLOCKS_PER_GROUP;
    uint32_t total_inodes = num_groups * EXT4_INODES_PER_GROUP;
    uint32_t reserved_blocks = num_blocks / 20; /* 5% reserved */

    printk(KERN_INFO "[EXT4] Blocks: %llu, Groups: %u, Inodes: %u\n",
           (unsigned long long)num_blocks, num_groups, total_inodes);

    /* Allocate buffers */
    void *block_buf = kmalloc(EXT4_BLOCK_SIZE);
    if (!block_buf) {
        printk(KERN_ERR "[EXT4] Failed to allocate buffer\n");
        return -1;
    }

    /* Step 1: Create superblock */
    memset(block_buf, 0, EXT4_BLOCK_SIZE);
    struct ext4_super *sb = (struct ext4_super *)block_buf;

    sb->s_inodes_count = total_inodes;
    sb->s_blocks_count_lo = (uint32_t)num_blocks;
    sb->s_r_blocks_count_lo = reserved_blocks;
    sb->s_free_blocks_count_lo = (uint32_t)(num_blocks - num_groups * 100); /* Rough estimate */
    sb->s_free_inodes_count = total_inodes - EXT4_RESERVED_INODES;
    sb->s_first_data_block = 0; /* 4K block size */
    sb->s_log_block_size = 2; /* log2(4096/1024) = 2 */
    sb->s_log_cluster_size = 2;
    sb->s_blocks_per_group = EXT4_BLOCKS_PER_GROUP;
    sb->s_clusters_per_group = EXT4_BLOCKS_PER_GROUP;
    sb->s_inodes_per_group = EXT4_INODES_PER_GROUP;
    sb->s_mtime = 0;
    sb->s_wtime = (uint32_t)arch_timer_get_ms() / 1000;
    sb->s_mnt_count = 0;
    sb->s_max_mnt_count = 65535;
    sb->s_magic = EXT4_SUPER_MAGIC;
    sb->s_state = 1; /* Clean */
    sb->s_errors = 1; /* Continue on errors */
    sb->s_minor_rev_level = 0;
    sb->s_lastcheck = sb->s_wtime;
    sb->s_checkinterval = 0;
    sb->s_creator_os = 0; /* Linux */
    sb->s_rev_level = 1; /* Dynamic */
    sb->s_def_resuid = 0;
    sb->s_def_resgid = 0;
    sb->s_first_ino = EXT4_GOOD_OLD_FIRST_INO;
    sb->s_inode_size = EXT4_INODE_SIZE;
    sb->s_block_group_nr = 0;
    sb->s_feature_compat = EXT4_FEATURE_COMPAT_RESIZE_INODE | EXT4_FEATURE_COMPAT_DIR_INDEX;
    sb->s_feature_incompat = EXT4_FEATURE_INCOMPAT_FILETYPE | EXT4_FEATURE_INCOMPAT_EXTENTS |
                              EXT4_FEATURE_INCOMPAT_64BIT | EXT4_FEATURE_INCOMPAT_FLEX_BG;
    sb->s_feature_ro_compat = EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER |
                               EXT4_FEATURE_RO_COMPAT_LARGE_FILE |
                               EXT4_FEATURE_RO_COMPAT_HUGE_FILE |
                               EXT4_FEATURE_RO_COMPAT_GDT_CSUM |
                               EXT4_FEATURE_RO_COMPAT_DIR_NLINK |
                               EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE;

    /* Copy volume label */
    strncpy(sb->s_volume_name, volume_label, 16);
    sb->s_volume_name[15] = '\0';

    sb->s_desc_size = sizeof(struct ext4_group_desc);
    sb->s_mkfs_time = sb->s_wtime;
    sb->s_min_extra_isize = 32;
    sb->s_want_extra_isize = 32;

    /* Write superblock at offset 1024 (block 0, offset 1024) */
    memset(block_buf, 0, 1024); /* Clear first 1024 bytes */
    memcpy((uint8_t *)block_buf + 1024, sb, sizeof(struct ext4_super));
    if (write_partition_block(dev, start_lba, 0, block_buf) < 0) {
        printk(KERN_ERR "[EXT4] Failed to write superblock\n");
        kfree(block_buf);
        return -1;
    }

    /* Step 2: Create group descriptors */
    for (uint32_t g = 0; g < num_groups; g++) {
        memset(block_buf, 0, EXT4_BLOCK_SIZE);
        struct ext4_group_desc *gdt = (struct ext4_group_desc *)block_buf;

        uint32_t blocks_in_group = EXT4_BLOCKS_PER_GROUP;
        if (g == num_groups - 1) {
            blocks_in_group = num_blocks - (g * EXT4_BLOCKS_PER_GROUP);
        }

        uint32_t group_start = g * EXT4_BLOCKS_PER_GROUP;
        uint32_t metadata_blocks = 10; /* Rough estimate for bitmaps + inode table */

        gdt->bg_block_bitmap_lo = group_start + 1;
        gdt->bg_inode_bitmap_lo = group_start + 2;
        gdt->bg_inode_table_lo = group_start + 3;
        gdt->bg_free_blocks_count_lo = blocks_in_group - metadata_blocks;
        gdt->bg_free_inodes_count_lo = EXT4_INODES_PER_GROUP;
        gdt->bg_used_dirs_count_lo = (g == 0) ? 2 : 0; /* Root and lost+found in group 0 */
        gdt->bg_itable_unused_lo = EXT4_INODES_PER_GROUP - ((g == 0) ? EXT4_RESERVED_INODES : 0);

        /* Write group descriptor to block 1 (or appropriate block for large fs) */
        if (g == 0) {
            if (write_partition_block(dev, start_lba, 1, block_buf) < 0) {
                printk(KERN_ERR "[EXT4] Failed to write group descriptor\n");
                kfree(block_buf);
                return -1;
            }
        }

        /* Initialize block bitmap (all blocks free) */
        memset(block_buf, 0xFF, EXT4_BLOCK_SIZE); /* All bits set = all free */
        if (write_partition_block(dev, start_lba, gdt->bg_block_bitmap_lo, block_buf) < 0) {
            kfree(block_buf);
            return -1;
        }

        /* Initialize inode bitmap */
        memset(block_buf, 0xFF, EXT4_BLOCK_SIZE); /* All inodes free */
        if (g == 0) {
            /* Reserve first 10 inodes */
            for (int i = 0; i < EXT4_RESERVED_INODES; i++) {
                ((uint8_t *)block_buf)[i / 8] &= ~(1 << (i % 8));
            }
        }
        if (write_partition_block(dev, start_lba, gdt->bg_inode_bitmap_lo, block_buf) < 0) {
            kfree(block_buf);
            return -1;
        }

        /* Initialize inode table (zero all inodes) */
        memset(block_buf, 0, EXT4_BLOCK_SIZE);
        uint32_t inode_table_blocks = (EXT4_INODES_PER_GROUP * EXT4_INODE_SIZE) / EXT4_BLOCK_SIZE;
        for (uint32_t i = 0; i < inode_table_blocks; i++) {
            if (write_partition_block(dev, start_lba, gdt->bg_inode_table_lo + i, block_buf) < 0) {
                kfree(block_buf);
                return -1;
            }
        }
    }

    /* Step 3: Create root directory (inode 2) */
    memset(block_buf, 0, EXT4_BLOCK_SIZE);
    struct ext4_inode *root_inode = (struct ext4_inode *)block_buf;
    root_inode->i_mode = EXT4_S_IFDIR | 0755;
    root_inode->i_uid = 0;
    root_inode->i_size_lo = EXT4_BLOCK_SIZE;
    root_inode->i_atime = root_inode->i_ctime = root_inode->i_mtime = sb->s_wtime;
    root_inode->i_links_count = 2; /* . and .. */
    root_inode->i_blocks_lo = (EXT4_BLOCK_SIZE / 512);
    root_inode->i_block[0] = 100; /* Data block for root directory */

    /* Write root inode to inode table (inode 2 is at offset 1 in the table, after inode 1) */
    uint32_t root_inode_block = 3 + (2 * EXT4_INODE_SIZE) / EXT4_BLOCK_SIZE;
    uint32_t root_inode_offset = (2 * EXT4_INODE_SIZE) % EXT4_BLOCK_SIZE;
    memcpy((uint8_t *)block_buf + root_inode_offset, root_inode, sizeof(struct ext4_inode));
    if (write_partition_block(dev, start_lba, root_inode_block, block_buf) < 0) {
        kfree(block_buf);
        return -1;
    }

    /* Create root directory data */
    memset(block_buf, 0, EXT4_BLOCK_SIZE);
    struct ext4_dir_entry *de = (struct ext4_dir_entry *)block_buf;

    /* Entry for "." */
    de->inode = EXT4_ROOT_INO;
    de->rec_len = 12;
    de->name_len = 1;
    de->file_type = 2; /* Directory */
    de->name[0] = '.';

    /* Entry for ".." */
    de = (struct ext4_dir_entry *)((uint8_t *)block_buf + 12);
    de->inode = EXT4_ROOT_INO;
    de->rec_len = EXT4_BLOCK_SIZE - 12;
    de->name_len = 2;
    de->file_type = 2;
    de->name[0] = '.';
    de->name[1] = '.';

    if (write_partition_block(dev, start_lba, 100, block_buf) < 0) {
        kfree(block_buf);
        return -1;
    }

    kfree(block_buf);

    printk(KERN_INFO "[EXT4] Filesystem created successfully\n");
    printk(KERN_INFO "[EXT4] Label: %s\n", volume_label);
    printk(KERN_INFO "[EXT4] Blocks: %llu, Inodes: %u\n",
           (unsigned long long)num_blocks, total_inodes);

    return 0;
}
