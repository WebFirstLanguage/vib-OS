/*
 * VMM stub for x86_64 installer
 */

#ifndef _VMM_H
#define _VMM_H

#include "types.h"

/* VM flags */
#define VM_READ    (1 << 0)
#define VM_WRITE   (1 << 1)
#define VM_EXEC    (1 << 2)
#define VM_DEVICE  (1 << 3)

/* Stub function */
static inline int vmm_map_range(uint64_t virt, uint64_t phys, size_t size, uint32_t flags) {
    (void)virt; (void)phys; (void)size; (void)flags;
    return 0;
}

#endif /* _VMM_H */
