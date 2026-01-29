/*
 * Architecture abstraction for x86_64
 */

#ifndef _ARCH_H
#define _ARCH_H

#include "types.h"

/* Timer functions */
static inline uint64_t arch_timer_get_ms(void) {
    /* Stub - return incrementing value */
    static uint64_t counter = 0;
    return counter++;
}

static inline void arch_irq_enable(void) {
    __asm__ volatile("sti");
}

static inline void arch_irq_disable(void) {
    __asm__ volatile("cli");
}

static inline void arch_halt(void) {
    while(1) {
        __asm__ volatile("hlt");
    }
}

#endif /* _ARCH_H */
