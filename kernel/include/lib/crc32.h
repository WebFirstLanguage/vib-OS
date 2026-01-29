/*
 * vib-OS CRC32 Checksum
 *
 * Implementation of CRC32 for GPT checksums
 */

#ifndef CRC32_H
#define CRC32_H

#include "types.h"

/* Calculate CRC32 checksum */
uint32_t crc32(uint32_t crc, const void *buf, size_t len);

/* Calculate CRC32 with initial value of 0 */
static inline uint32_t crc32_compute(const void *buf, size_t len) {
    return crc32(0, buf, len);
}

#endif /* CRC32_H */
