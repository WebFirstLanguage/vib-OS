/*
 * vib-OS Boot Parameters Parser
 */

#include "core/boot_params.h"
#include "printk.h"
#include "string.h"
#include "types.h"

/* Global boot parameters */
struct boot_params boot_params = {
    .live_boot = false,
    .root_device = "",
    .cmdline = "",
};

/* Helper: Find parameter value in command line */
static const char *find_param(const char *cmdline, const char *param) {
    if (!cmdline || !param) return NULL;

    const char *p = cmdline;
    size_t param_len = strlen(param);

    while (*p) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') p++;

        /* Check if this is our parameter */
        if (strncmp(p, param, param_len) == 0) {
            p += param_len;
            if (*p == '=') {
                return p + 1;  /* Return value after '=' */
            } else if (*p == ' ' || *p == '\t' || *p == '\0') {
                return "";  /* Boolean flag (no value) */
            }
        }

        /* Move to next parameter */
        while (*p && *p != ' ' && *p != '\t') p++;
    }

    return NULL;
}

/* Helper: Copy parameter value until space/end */
static void copy_param_value(char *dest, const char *src, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1 && src[i] && src[i] != ' ' && src[i] != '\t') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/*
 * boot_params_init - Parse kernel command line
 * @cmdline: Kernel command line string (e.g., from GRUB or device tree)
 */
void boot_params_init(const char *cmdline) {
    /* Clear boot params */
    boot_params.live_boot = false;
    boot_params.root_device[0] = '\0';
    boot_params.cmdline[0] = '\0';

    if (!cmdline) {
        printk(KERN_INFO "[BOOT] No command line provided\n");
        return;
    }

    /* Store full command line */
    size_t cmdline_len = strlen(cmdline);
    if (cmdline_len >= sizeof(boot_params.cmdline)) {
        cmdline_len = sizeof(boot_params.cmdline) - 1;
    }
    strncpy(boot_params.cmdline, cmdline, cmdline_len);
    boot_params.cmdline[cmdline_len] = '\0';

    printk(KERN_INFO "[BOOT] Command line: %s\n", cmdline);

    /* Parse live_boot parameter */
    const char *live = find_param(cmdline, "live_boot");
    if (live) {
        if (*live == '\0' || *live == '1' ||
            strncmp(live, "true", 4) == 0 ||
            strncmp(live, "yes", 3) == 0) {
            boot_params.live_boot = true;
            printk(KERN_INFO "[BOOT] Live boot mode enabled\n");
        }
    }

    /* Parse root device parameter */
    const char *root = find_param(cmdline, "root");
    if (root && *root) {
        copy_param_value(boot_params.root_device, root,
                        sizeof(boot_params.root_device));
        printk(KERN_INFO "[BOOT] Root device: %s\n", boot_params.root_device);
    }
}

/*
 * Query functions
 */

bool boot_is_live(void) {
    return boot_params.live_boot;
}

const char *boot_get_root_device(void) {
    return boot_params.root_device;
}

const char *boot_get_cmdline(void) {
    return boot_params.cmdline;
}
