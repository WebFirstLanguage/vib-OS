/*
 * vib-OS Boot Parameters Parser
 *
 * Parse kernel command-line parameters passed by bootloader
 */

#ifndef BOOT_PARAMS_H
#define BOOT_PARAMS_H

#include "types.h"

/* Boot parameter structure */
struct boot_params {
    bool live_boot;           /* Running in live-boot mode */
    char root_device[64];     /* Root device path (e.g., "/dev/vda2") */
    char cmdline[256];        /* Full command line */
};

/* Global boot parameters */
extern struct boot_params boot_params;

/* Initialize boot parameter parsing */
void boot_params_init(const char *cmdline);

/* Query functions */
bool boot_is_live(void);
const char *boot_get_root_device(void);
const char *boot_get_cmdline(void);

#endif /* BOOT_PARAMS_H */
