/*
 * vib-OS Installer File Copy Engine
 */

#include "installer/file_copy.h"
#include "fs/vfs.h"
#include "mm/kmalloc.h"
#include "printk.h"
#include "string.h"
#include "types.h"

#define COPY_BUFFER_SIZE (64 * 1024) /* 64KB copy buffer */
#define MAX_DIR_ENTRIES 256

/* Directory entry list for readdir callback */
struct dir_entry_list {
    char names[MAX_DIR_ENTRIES][NAME_MAX + 1];
    int count;
    const char *parent_path;
};

/* Forward declarations */
static int copy_directory(const char *src_path, const char *dst_path,
                          uint64_t *copied_bytes, uint64_t total_bytes,
                          progress_callback_t progress_cb);

static int copy_file(const char *src_path, const char *dst_path, size_t size,
                     uint64_t *copied_bytes, uint64_t total_bytes,
                     progress_callback_t progress_cb);

/* Callback for vfs_readdir */
static int filldir_callback(void *ctx, const char *name, int namelen,
                            loff_t offset, ino_t ino, unsigned d_type) {
    (void)offset;
    (void)ino;
    (void)d_type;

    struct dir_entry_list *list = (struct dir_entry_list *)ctx;
    if (list->count >= MAX_DIR_ENTRIES) {
        return -1; /* List full */
    }

    if (namelen > NAME_MAX) namelen = NAME_MAX;
    memcpy(list->names[list->count], name, namelen);
    list->names[list->count][namelen] = '\0';
    list->count++;

    return 0;
}

/* Helper: Get file size from struct file */
static size_t get_file_size(struct file *file) {
    if (!file || !file->f_dentry || !file->f_dentry->d_inode) {
        return 0;
    }
    return (size_t)file->f_dentry->d_inode->i_size;
}

/* Helper: Check if file is a directory */
static bool is_directory(struct file *file) {
    if (!file || !file->f_dentry || !file->f_dentry->d_inode) {
        return false;
    }
    return S_ISDIR(file->f_dentry->d_inode->i_mode);
}

/* Helper: Check if file is a regular file */
static bool is_regular_file(struct file *file) {
    if (!file || !file->f_dentry || !file->f_dentry->d_inode) {
        return false;
    }
    return S_ISREG(file->f_dentry->d_inode->i_mode);
}

/*
 * calculate_total_size - Calculate total size of directory tree
 */
uint64_t calculate_total_size(const char *path) {
    /* Try to open as file first */
    struct file *file = vfs_open(path, O_RDONLY, 0);
    if (!file) {
        return 0;
    }

    uint64_t total = 0;

    if (is_regular_file(file)) {
        /* Regular file - return its size */
        total = get_file_size(file);
        vfs_close(file);
        return total;
    }

    if (!is_directory(file)) {
        /* Not a file or directory */
        vfs_close(file);
        return 0;
    }

    /* It's a directory - read entries */
    struct dir_entry_list list;
    list.count = 0;
    list.parent_path = path;

    vfs_readdir(file, &list, filldir_callback);
    vfs_close(file);

    /* Process entries */
    for (int i = 0; i < list.count; i++) {
        /* Skip . and .. */
        if (strcmp(list.names[i], ".") == 0 ||
            strcmp(list.names[i], "..") == 0) {
            continue;
        }

        /* Build full path */
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, list.names[i]);

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
    struct file *src_file = vfs_open(src_path, O_RDONLY, 0);
    if (!src_file) {
        printk(KERN_ERR "[COPY] Failed to open source: %s\n", src_path);
        return -1;
    }

    /* Create destination file */
    struct file *dst_file = vfs_open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
    if (vfs_mkdir(dst_path, 0755) < 0) {
        printk(KERN_ERR "[COPY] Failed to create directory: %s\n", dst_path);
        return -1;
    }

    printk(KERN_DEBUG "[COPY] Created directory: %s\n", dst_path);

    /* Open source directory */
    struct file *src_dir = vfs_open(src_path, O_RDONLY | O_DIRECTORY, 0);
    if (!src_dir) {
        printk(KERN_ERR "[COPY] Failed to open directory: %s\n", src_path);
        return -1;
    }

    /* Read directory entries */
    struct dir_entry_list list;
    list.count = 0;
    list.parent_path = src_path;

    vfs_readdir(src_dir, &list, filldir_callback);
    vfs_close(src_dir);

    int ret = 0;

    /* Process entries */
    for (int i = 0; i < list.count; i++) {
        /* Skip . and .. */
        if (strcmp(list.names[i], ".") == 0 ||
            strcmp(list.names[i], "..") == 0) {
            continue;
        }

        /* Build full paths */
        char src_full[PATH_MAX];
        char dst_full[PATH_MAX];
        snprintf(src_full, sizeof(src_full), "%s/%s", src_path, list.names[i]);
        snprintf(dst_full, sizeof(dst_full), "%s/%s", dst_path, list.names[i]);

        /* Open file to check type */
        struct file *file = vfs_open(src_full, O_RDONLY, 0);
        if (!file) {
            printk(KERN_WARNING "[COPY] Failed to open: %s\n", src_full);
            continue;
        }

        if (is_directory(file)) {
            /* Recursively copy subdirectory */
            vfs_close(file);

            ret = copy_directory(src_full, dst_full, copied_bytes,
                                total_bytes, progress_cb);
            if (ret < 0) {
                break;
            }
        } else if (is_regular_file(file)) {
            /* Copy regular file */
            size_t file_size = get_file_size(file);
            vfs_close(file);

            ret = copy_file(src_full, dst_full, file_size, copied_bytes,
                           total_bytes, progress_cb);
            if (ret < 0) {
                break;
            }
        } else {
            /* Skip special files */
            vfs_close(file);
            printk(KERN_DEBUG "[COPY] Skipping special file: %s\n", src_full);
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
