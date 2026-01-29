/*
 * Minimal printk for x86_64 build
 */

#ifndef _PRINTK_H
#define _PRINTK_H

/* Log levels (stubs for compatibility) */
#define KERN_EMERG   "<0>"
#define KERN_ALERT   "<1>"
#define KERN_CRIT    "<2>"
#define KERN_ERR     "<3>"
#define KERN_WARNING "<4>"
#define KERN_NOTICE  "<5>"
#define KERN_INFO    "<6>"
#define KERN_DEBUG   "<7>"

/* printk stub - does nothing in minimal x86_64 build */
static inline int printk(const char *fmt, ...) {
    (void)fmt;
    return 0;
}

#endif /* _PRINTK_H */
