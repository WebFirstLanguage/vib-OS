/*
 * vib-OS Installer File Copy Engine (x86_64 version)
 */

#include "installer/file_copy.h"
#include "vfs.h"
#include "kmalloc.h"
#include "printk.h"
#include "string.h"
#include "types.h"

#define COPY_BUFFER_SIZE (64 * 1024) /* 64KB copy buffer */

/* Forward declarations */
static int copy_directory(const char *src_path, const char *dst_path,
                          uint64_t *copied_bytes, uint64_t total_bytes,
                          progress_callback_t progress_cb);

static int copy_file(const char *src_path, const char *dst_path, size_t size,
                     uint64_t *copied_bytes, uint64_t total_bytes,
                     progress_callback_t progress_cb);

/*
 * calculate_total_size - Calculate total size of directory tree
 */
uint64_t calculate_total_size(const char *path) {
    size_t file_size;
    int is_dir;

    /* Check if file/directory exists */
    if (vfs_stat(path, &file_size, &is_dir) < 0) {
        return 0;
    }

    if (!is_dir) {
        /* Regular file */
        return file_size;
    }

    /* It's a directory - enumerate entries */
    dirent_t entries[256];
    int num_entries = vfs_readdir(path, entries, 256);

    uint64_t total = 0;
    for (int i = 0; i < num_entries; i++) {
        /* Skip . and .. */
        if (strcmp(entries[i].name, ".") == 0 ||
            strcmp(entries[i].name, "..") == 0) {
            continue;
        }

        /* Build full path */
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entries[i].name);

        /* Recursively calculate size */
        total += calculate_total_size(full_path);
    }

    return total;
}

/*
 * copy_file - Copy single file
 */
static int copy_file(const char *src_path, const char *dst_path, size_t size,
                     uint64_t *copied_bytes, uint64_t total_bytes,
                     progress_callback_t progress_cb) {
    /* Open source file */
    file_t *src_file = vfs_open(src_path, O_RDONLY);
    if (!src_file) {
        printk(KERN_ERR "[COPY] Failed to open source: %s\n", src_path);
        return -1;
    }

    /* Create destination file */
    file_t *dst_file = vfs_open(dst_path, O_WRONLY | O_CREAT | O_TRUNC);
    if (!dst_file) {
        printk(KERN_ERR "[COPY] Failed to create dest: %s\n", dst_path);
        vfs_close(src_file);
        return -1;
    }

    /* Allocate copy buffer */
    char *buffer = kmalloc(COPY_BUFFER_SIZE);
    if (!buffer) {
        printk(KERN_ERR "[COPY] Failed to allocate buffer\n");
        vfs_close(src_file);
        vfs_close(dst_file);
        return -1;
    }

    /* Copy file in chunks */
    size_t remaining = size;
    while (remaining > 0) {
        size_t chunk_size = (remaining < COPY_BUFFER_SIZE) ? remaining : COPY_BUFFER_SIZE;

        ssize_t bytes_read = vfs_read(src_file, buffer, chunk_size);
        if (bytes_read <= 0) {
            printk(KERN_ERR "[COPY] Read error from %s\n", src_path);
            kfree(buffer);
            vfs_close(src_file);
            vfs_close(dst_file);
            return -1;
        }

        ssize_t bytes_written = vfs_write(dst_file, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            printk(KERN_ERR "[COPY] Write error to %s\n", dst_path);
            kfree(buffer);
            vfs_close(src_file);
            vfs_close(dst_file);
            return -1;
        }

        remaining -= bytes_read;
        *copied_bytes += bytes_read;

        /* Call progress callback */
        if (progress_cb) {
            progress_cb(*copied_bytes, total_bytes, src_path);
        }
    }

    kfree(buffer);
    vfs_close(src_file);
    vfs_close(dst_file);

    printk(KERN_DEBUG "[COPY] Copied: %s -> %s (%zu bytes)\n",
           src_path, dst_path, size);

    return 0;
}

/*
 * copy_directory - Recursively copy directory
 */
static int copy_directory(const char *src_path, const char *dst_path,
                          uint64_t *copied_bytes, uint64_t total_bytes,
                          progress_callback_t progress_cb) {
    /* Create destination directory */
    if (vfs_mkdir(dst_path) < 0) {
        printk(KERN_ERR "[COPY] Failed to create directory: %s\n", dst_path);
        return -1;
    }

    printk(KERN_DEBUG "[COPY] Created directory: %s\n", dst_path);

    /* Read directory entries */
    dirent_t entries[256];
    int num_entries = vfs_readdir(src_path, entries, 256);

    if (num_entries < 0) {
        printk(KERN_ERR "[COPY] Failed to read directory: %s\n", src_path);
        return -1;
    }

    int ret = 0;

    /* Process entries */
    for (int i = 0; i < num_entries; i++) {
        /* Skip . and .. */
        if (strcmp(entries[i].name, ".") == 0 ||
            strcmp(entries[i].name, "..") == 0) {
            continue;
        }

        /* Build full paths */
        char src_full[MAX_PATH];
        char dst_full[MAX_PATH];
        snprintf(src_full, sizeof(src_full), "%s/%s", src_path, entries[i].name);
        snprintf(dst_full, sizeof(dst_full), "%s/%s", dst_path, entries[i].name);

        if (entries[i].type == 1) {
            /* Directory */
            ret = copy_directory(src_full, dst_full, copied_bytes,
                                total_bytes, progress_cb);
            if (ret < 0) {
                break;
            }
        } else {
            /* Regular file */
            ret = copy_file(src_full, dst_full, entries[i].size, copied_bytes,
                           total_bytes, progress_cb);
            if (ret < 0) {
                break;
            }
        }
    }

    return ret;
}

/*
 * copy_filesystem - Main entry point for filesystem copy
 */
int copy_filesystem(const char *src_root, const char *dst_root,
                    progress_callback_t progress_cb) {
    if (!src_root || !dst_root) {
        return -1;
    }

    printk(KERN_INFO "[COPY] Starting filesystem copy: %s -> %s\n",
           src_root, dst_root);

    /* Calculate total size */
    printk(KERN_INFO "[COPY] Calculating total size...\n");
    uint64_t total_bytes = calculate_total_size(src_root);
    printk(KERN_INFO "[COPY] Total size: %llu MB\n",
           (unsigned long long)(total_bytes / (1024 * 1024)));

    /* Start copy */
    uint64_t copied_bytes = 0;
    int ret = copy_directory(src_root, dst_root, &copied_bytes, total_bytes,
                            progress_cb);

    if (ret == 0) {
        printk(KERN_INFO "[COPY] Copy completed successfully\n");
        printk(KERN_INFO "[COPY] Copied %llu MB\n",
               (unsigned long long)(copied_bytes / (1024 * 1024)));
    } else {
        printk(KERN_ERR "[COPY] Copy failed\n");
    }

    return ret;
}
