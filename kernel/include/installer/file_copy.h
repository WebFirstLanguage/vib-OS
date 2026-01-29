/*
 * vib-OS Installer File Copy Engine
 *
 * Recursively copy files from source to destination filesystem
 */

#ifndef FILE_COPY_H
#define FILE_COPY_H

#include "types.h"

/* Progress callback function */
typedef void (*progress_callback_t)(uint64_t copied, uint64_t total,
                                     const char *current_file);

/*
 * copy_filesystem - Recursively copy filesystem tree
 * @src_root: Source root path (e.g., "/")
 * @dst_root: Destination root path (mounted partition)
 * @progress_cb: Optional progress callback
 *
 * Returns: 0 on success, negative on error
 */
int copy_filesystem(const char *src_root, const char *dst_root,
                    progress_callback_t progress_cb);

/*
 * calculate_total_size - Calculate total size of directory tree
 * @path: Directory path to measure
 *
 * Returns: Total size in bytes
 */
uint64_t calculate_total_size(const char *path);

#endif /* FILE_COPY_H */
