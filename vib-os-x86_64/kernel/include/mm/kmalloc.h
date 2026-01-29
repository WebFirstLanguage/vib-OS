/*
 * kmalloc compatibility header for x86_64
 */

#ifndef _KMALLOC_H
#define _KMALLOC_H

#include "types.h"

/* Use existing kmalloc from x86_64 mm/kmalloc.c */
void *kmalloc(size_t size);
void kfree(void *ptr);

#endif /* _KMALLOC_H */
