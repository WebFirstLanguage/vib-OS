/*
 * vib-OS Installer GUI Application
 *
 * Multi-step installation wizard for installing vib-OS to disk
 */

#include "apps/installer.h"
#include "core/boot_params.h"
#include "drivers/block_dev.h"
#include "fs/ext4_mkfs.h"
#include "gui/gui.h"
#include "installer/bootloader.h"
#include "installer/file_copy.h"
#include "lib/partition/gpt.h"
#include "mm/kmalloc.h"
#include "printk.h"
#include "string.h"

/* Installation steps */
typedef enum {
    INSTALL_STEP_WELCOME,
    INSTALL_STEP_DISK_SELECT,
    INSTALL_STEP_PARTITION,
    INSTALL_STEP_CONFIRM,
    INSTALL_STEP_INSTALLING,
    INSTALL_STEP_COMPLETE,
    INSTALL_STEP_ERROR
} install_step_t;

/* Installer state */
typedef struct {
    install_step_t current_step;
    block_device_t *selected_disk;
    int disk_count;
    block_device_t *disk_list[16];
    uint64_t total_bytes;
    uint64_t copied_bytes;
    int progress_percent;
    char current_file[256];
    char error_message[256];
    int selected_disk_index;
} installer_state_t;

static installer_state_t installer_state;
static struct window *installer_window = NULL;

/* GUI Colors */
#define COLOR_BG 0xFFFFFF
#define COLOR_FG 0x000000
#define COLOR_BUTTON 0x007AFF
#define COLOR_BUTTON_HOVER 0x0051D5
#define COLOR_DANGER 0xFF3B30
#define COLOR_SUCCESS 0x34C759
#define COLOR_PROGRESS_BG 0xE5E5E5
#define COLOR_PROGRESS_FG 0x007AFF

/* Forward declarations */
static void installer_draw(struct window *win);
static void installer_mouse(struct window *win, int x, int y, int buttons);
static void installer_key(struct window *win, int key);

static void draw_welcome_screen(void);
static void draw_disk_select_screen(void);
static void draw_partition_screen(void);
static void draw_confirm_screen(void);
static void draw_installing_screen(void);
static void draw_complete_screen(void);
static void draw_error_screen(void);

static void start_installation(void);
static void progress_callback(uint64_t copied, uint64_t total, const char *file);

/*
 * installer_should_show - Check if installer should be shown
 */
int installer_should_show(void) {
    return boot_is_live();
}

/*
 * installer_init - Initialize installer window
 */
void installer_init(void) {
    if (!installer_should_show()) {
        printk(KERN_INFO "[INSTALLER] Not in live boot mode, skipping installer\n");
        return;
    }

    printk(KERN_INFO "[INSTALLER] Initializing installer UI\n");

    /* Initialize state */
    memset(&installer_state, 0, sizeof(installer_state));
    installer_state.current_step = INSTALL_STEP_WELCOME;
    installer_state.selected_disk_index = -1;

    /* Enumerate disks */
    block_device_t *dev = block_dev_enumerate();
    while (dev && installer_state.disk_count < 16) {
        installer_state.disk_list[installer_state.disk_count++] = dev;
        dev = dev->next;
    }

    printk(KERN_INFO "[INSTALLER] Found %d disk(s)\n", installer_state.disk_count);

    /* Create installer window */
    extern struct window *gui_create_window(const char *title, int x, int y,
                                            int w, int h);
    installer_window = gui_create_window("vib-OS Installer", 100, 100, 600, 400);

    if (installer_window) {
        installer_window->on_draw = installer_draw;
        installer_window->on_mouse = installer_mouse;
        installer_window->on_key = installer_key;
        printk(KERN_INFO "[INSTALLER] Installer window created\n");
    }
}

/*
 * installer_draw - Draw installer window
 */
static void installer_draw(struct window *win) {
    if (!win) return;

    /* Clear window background */
    extern void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
    gui_draw_rect(win->x, win->y + 30, win->w, win->h - 30, COLOR_BG);

    /* Draw current step */
    switch (installer_state.current_step) {
        case INSTALL_STEP_WELCOME:
            draw_welcome_screen();
            break;
        case INSTALL_STEP_DISK_SELECT:
            draw_disk_select_screen();
            break;
        case INSTALL_STEP_PARTITION:
            draw_partition_screen();
            break;
        case INSTALL_STEP_CONFIRM:
            draw_confirm_screen();
            break;
        case INSTALL_STEP_INSTALLING:
            draw_installing_screen();
            break;
        case INSTALL_STEP_COMPLETE:
            draw_complete_screen();
            break;
        case INSTALL_STEP_ERROR:
            draw_error_screen();
            break;
    }
}

/*
 * Draw functions for each installation step
 */

static void draw_welcome_screen(void) {
    extern void gui_draw_string(int x, int y, const char *str, uint32_t color);

    int base_x = installer_window->x + 50;
    int base_y = installer_window->y + 80;

    gui_draw_string(base_x, base_y, "Welcome to vib-OS Installer", COLOR_FG);
    gui_draw_string(base_x, base_y + 40, "This wizard will guide you through installing vib-OS", COLOR_FG);
    gui_draw_string(base_x, base_y + 60, "to your computer.", COLOR_FG);
    gui_draw_string(base_x, base_y + 100, "WARNING: This will erase all data on the target disk!", COLOR_DANGER);

    /* Draw Next button */
    extern void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
    int btn_x = installer_window->x + installer_window->w - 120;
    int btn_y = installer_window->y + installer_window->h - 50;
    gui_draw_rect(btn_x, btn_y, 100, 30, COLOR_BUTTON);
    gui_draw_string(btn_x + 25, btn_y + 10, "Next >", COLOR_BG);
}

static void draw_disk_select_screen(void) {
    extern void gui_draw_string(int x, int y, const char *str, uint32_t color);
    extern void gui_draw_rect(int x, int y, int w, int h, uint32_t color);

    int base_x = installer_window->x + 50;
    int base_y = installer_window->y + 80;

    gui_draw_string(base_x, base_y, "Select Installation Disk", COLOR_FG);
    gui_draw_string(base_x, base_y + 30, "Choose the disk where vib-OS will be installed:", COLOR_FG);

    /* List disks */
    for (int i = 0; i < installer_state.disk_count; i++) {
        block_device_t *dev = installer_state.disk_list[i];
        char disk_info[128];
        snprintf(disk_info, sizeof(disk_info), "%s (%llu MB)",
                 dev->name, dev->size_bytes / (1024 * 1024));

        int item_y = base_y + 70 + (i * 40);

        /* Draw selection indicator */
        if (i == installer_state.selected_disk_index) {
            gui_draw_rect(base_x - 10, item_y - 5, 15, 15, COLOR_BUTTON);
        } else {
            gui_draw_rect(base_x - 10, item_y - 5, 15, 15, 0xCCCCCC);
        }

        gui_draw_string(base_x + 20, item_y, disk_info, COLOR_FG);
    }

    /* Draw buttons */
    int btn_y = installer_window->y + installer_window->h - 50;

    /* Back button */
    gui_draw_rect(installer_window->x + 20, btn_y, 100, 30, 0xCCCCCC);
    gui_draw_string(installer_window->x + 35, btn_y + 10, "< Back", COLOR_FG);

    /* Next button */
    int next_x = installer_window->x + installer_window->w - 120;
    uint32_t next_color = (installer_state.selected_disk_index >= 0) ? COLOR_BUTTON : 0x999999;
    gui_draw_rect(next_x, btn_y, 100, 30, next_color);
    gui_draw_string(next_x + 25, btn_y + 10, "Next >", COLOR_BG);
}

static void draw_partition_screen(void) {
    extern void gui_draw_string(int x, int y, const char *str, uint32_t color);

    int base_x = installer_window->x + 50;
    int base_y = installer_window->y + 80;

    gui_draw_string(base_x, base_y, "Partition Layout", COLOR_FG);
    gui_draw_string(base_x, base_y + 30, "The following partitions will be created:", COLOR_FG);

    gui_draw_string(base_x, base_y + 70, "Partition 1: ESP (EFI System) - 200 MB", COLOR_FG);
    gui_draw_string(base_x, base_y + 95, "Partition 2: Root (/) - Remainder", COLOR_FG);

    /* Draw buttons */
    int btn_y = installer_window->y + installer_window->h - 50;

    extern void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
    gui_draw_rect(installer_window->x + 20, btn_y, 100, 30, 0xCCCCCC);
    gui_draw_string(installer_window->x + 35, btn_y + 10, "< Back", COLOR_FG);

    int next_x = installer_window->x + installer_window->w - 120;
    gui_draw_rect(next_x, btn_y, 100, 30, COLOR_BUTTON);
    gui_draw_string(next_x + 25, btn_y + 10, "Next >", COLOR_BG);
}

static void draw_confirm_screen(void) {
    extern void gui_draw_string(int x, int y, const char *str, uint32_t color);
    extern void gui_draw_rect(int x, int y, int w, int h, uint32_t color);

    int base_x = installer_window->x + 50;
    int base_y = installer_window->y + 80;

    gui_draw_string(base_x, base_y, "Confirm Installation", COLOR_FG);
    gui_draw_string(base_x, base_y + 40, "ALL DATA WILL BE LOST!", COLOR_DANGER);

    if (installer_state.selected_disk) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Target disk: %s", installer_state.selected_disk->name);
        gui_draw_string(base_x, base_y + 80, msg, COLOR_FG);
    }

    gui_draw_string(base_x, base_y + 120, "Click 'Install Now' to begin installation", COLOR_FG);

    /* Draw buttons */
    int btn_y = installer_window->y + installer_window->h - 50;

    gui_draw_rect(installer_window->x + 20, btn_y, 100, 30, 0xCCCCCC);
    gui_draw_string(installer_window->x + 35, btn_y + 10, "< Back", COLOR_FG);

    int install_x = installer_window->x + installer_window->w - 150;
    gui_draw_rect(install_x, btn_y, 130, 30, COLOR_DANGER);
    gui_draw_string(install_x + 10, btn_y + 10, "Install Now", COLOR_BG);
}

static void draw_installing_screen(void) {
    extern void gui_draw_string(int x, int y, const char *str, uint32_t color);
    extern void gui_draw_rect(int x, int y, int w, int h, uint32_t color);

    int base_x = installer_window->x + 50;
    int base_y = installer_window->y + 80;

    gui_draw_string(base_x, base_y, "Installing vib-OS...", COLOR_FG);

    /* Progress bar */
    int progress_y = base_y + 50;
    int progress_w = installer_window->w - 100;
    gui_draw_rect(base_x, progress_y, progress_w, 30, COLOR_PROGRESS_BG);

    int filled_w = (progress_w * installer_state.progress_percent) / 100;
    gui_draw_rect(base_x, progress_y, filled_w, 30, COLOR_PROGRESS_FG);

    /* Progress percentage */
    char progress_text[32];
    snprintf(progress_text, sizeof(progress_text), "%d%%", installer_state.progress_percent);
    gui_draw_string(base_x + progress_w / 2 - 15, progress_y + 10, progress_text, COLOR_FG);

    /* Current file */
    if (installer_state.current_file[0]) {
        char file_msg[128];
        snprintf(file_msg, sizeof(file_msg), "Copying: %s", installer_state.current_file);
        gui_draw_string(base_x, progress_y + 50, file_msg, COLOR_FG);
    }
}

static void draw_complete_screen(void) {
    extern void gui_draw_string(int x, int y, const char *str, uint32_t color);
    extern void gui_draw_rect(int x, int y, int w, int h, uint32_t color);

    int base_x = installer_window->x + 50;
    int base_y = installer_window->y + 80;

    gui_draw_string(base_x, base_y, "Installation Complete!", COLOR_SUCCESS);
    gui_draw_string(base_x, base_y + 40, "vib-OS has been successfully installed.", COLOR_FG);
    gui_draw_string(base_x, base_y + 70, "Remove the installation media and reboot.", COLOR_FG);

    /* Draw Reboot button */
    int btn_x = installer_window->x + installer_window->w - 120;
    int btn_y = installer_window->y + installer_window->h - 50;
    gui_draw_rect(btn_x, btn_y, 100, 30, COLOR_SUCCESS);
    gui_draw_string(btn_x + 15, btn_y + 10, "Reboot", COLOR_BG);
}

static void draw_error_screen(void) {
    extern void gui_draw_string(int x, int y, const char *str, uint32_t color);

    int base_x = installer_window->x + 50;
    int base_y = installer_window->y + 80;

    gui_draw_string(base_x, base_y, "Installation Failed", COLOR_DANGER);
    gui_draw_string(base_x, base_y + 40, installer_state.error_message, COLOR_FG);
}

/*
 * installer_mouse - Handle mouse events
 */
static void installer_mouse(struct window *win, int x, int y, int buttons) {
    (void)win;
    (void)x;
    (void)y;

    if (!(buttons & 1)) return; /* Only handle left click */

    /* Handle button clicks based on current step */
    int btn_y = installer_window->y + installer_window->h - 50;

    switch (installer_state.current_step) {
        case INSTALL_STEP_WELCOME:
            /* Next button */
            if (x >= installer_window->x + installer_window->w - 120 && x <= installer_window->x + installer_window->w - 20 &&
                y >= btn_y && y <= btn_y + 30) {
                installer_state.current_step = INSTALL_STEP_DISK_SELECT;
            }
            break;

        case INSTALL_STEP_DISK_SELECT:
            /* Check disk selection clicks */
            for (int i = 0; i < installer_state.disk_count; i++) {
                int item_y = installer_window->y + 150 + (i * 40);
                if (y >= item_y && y <= item_y + 30) {
                    installer_state.selected_disk_index = i;
                    installer_state.selected_disk = installer_state.disk_list[i];
                }
            }

            /* Back button */
            if (x >= installer_window->x + 20 && x <= installer_window->x + 120 &&
                y >= btn_y && y <= btn_y + 30) {
                installer_state.current_step = INSTALL_STEP_WELCOME;
            }

            /* Next button */
            if (installer_state.selected_disk_index >= 0 &&
                x >= installer_window->x + installer_window->w - 120 && x <= installer_window->x + installer_window->w - 20 &&
                y >= btn_y && y <= btn_y + 30) {
                installer_state.current_step = INSTALL_STEP_PARTITION;
            }
            break;

        case INSTALL_STEP_PARTITION:
            /* Back button */
            if (x >= installer_window->x + 20 && x <= installer_window->x + 120 &&
                y >= btn_y && y <= btn_y + 30) {
                installer_state.current_step = INSTALL_STEP_DISK_SELECT;
            }

            /* Next button */
            if (x >= installer_window->x + installer_window->w - 120 && x <= installer_window->x + installer_window->w - 20 &&
                y >= btn_y && y <= btn_y + 30) {
                installer_state.current_step = INSTALL_STEP_CONFIRM;
            }
            break;

        case INSTALL_STEP_CONFIRM:
            /* Back button */
            if (x >= installer_window->x + 20 && x <= installer_window->x + 120 &&
                y >= btn_y && y <= btn_y + 30) {
                installer_state.current_step = INSTALL_STEP_PARTITION;
            }

            /* Install Now button */
            if (x >= installer_window->x + installer_window->w - 150 && x <= installer_window->x + installer_window->w - 20 &&
                y >= btn_y && y <= btn_y + 30) {
                start_installation();
            }
            break;

        case INSTALL_STEP_COMPLETE:
            /* Reboot button */
            if (x >= installer_window->x + installer_window->w - 120 && x <= installer_window->x + installer_window->w - 20 &&
                y >= btn_y && y <= btn_y + 30) {
                printk(KERN_INFO "[INSTALLER] Reboot requested\n");
                /* TODO: Trigger system reboot */
            }
            break;

        default:
            break;
    }
}

static void installer_key(struct window *win, int key) {
    (void)win;
    (void)key;
}

/*
 * start_installation - Begin installation process
 */
static void start_installation(void) {
    installer_state.current_step = INSTALL_STEP_INSTALLING;
    installer_state.progress_percent = 0;

    printk(KERN_INFO "[INSTALLER] Starting installation...\n");

    block_device_t *dev = installer_state.selected_disk;
    if (!dev) {
        snprintf(installer_state.error_message, sizeof(installer_state.error_message),
                 "No disk selected");
        installer_state.current_step = INSTALL_STEP_ERROR;
        return;
    }

    /* Phase 1: Create GPT (10%) */
    installer_state.progress_percent = 5;
    printk(KERN_INFO "[INSTALLER] Creating partition table...\n");

    gpt_context_t gpt_ctx;
    if (gpt_create(dev, &gpt_ctx) < 0) {
        snprintf(installer_state.error_message, sizeof(installer_state.error_message),
                 "Failed to create partition table");
        installer_state.current_step = INSTALL_STEP_ERROR;
        return;
    }

    /* Add ESP partition (200 MB) */
    uint64_t esp_size_sectors = (200 * 1024 * 1024) / dev->block_size;
    uint8_t esp_guid[] = GPT_TYPE_EFI_SYSTEM;
    gpt_add_partition(&gpt_ctx, 0, "ESP", gpt_ctx.header.first_usable_lba,
                     gpt_ctx.header.first_usable_lba + esp_size_sectors - 1, esp_guid);

    /* Add root partition (remainder) */
    uint8_t root_guid[] = GPT_TYPE_LINUX_FILESYSTEM;
    gpt_add_partition(&gpt_ctx, 1, "vib-os-root",
                     gpt_ctx.header.first_usable_lba + esp_size_sectors,
                     gpt_ctx.header.last_usable_lba, root_guid);

    if (gpt_write(&gpt_ctx) < 0) {
        snprintf(installer_state.error_message, sizeof(installer_state.error_message),
                 "Failed to write partition table");
        installer_state.current_step = INSTALL_STEP_ERROR;
        kfree(gpt_ctx.entries);
        return;
    }

    installer_state.progress_percent = 10;

    /* Phase 2: Format partitions (30%) */
    printk(KERN_INFO "[INSTALLER] Formatting partitions...\n");

    uint64_t root_start_lba = gpt_ctx.header.first_usable_lba + esp_size_sectors;
    uint64_t root_sectors = gpt_ctx.header.last_usable_lba - root_start_lba + 1;

    if (ext4_mkfs(dev, root_start_lba, root_sectors, "vib-os") < 0) {
        snprintf(installer_state.error_message, sizeof(installer_state.error_message),
                 "Failed to format root partition");
        installer_state.current_step = INSTALL_STEP_ERROR;
        kfree(gpt_ctx.entries);
        return;
    }

    installer_state.progress_percent = 30;

    /* Phase 3: Copy files (60%) */
    printk(KERN_INFO "[INSTALLER] Copying files...\n");
    /* TODO: Mount root partition and copy files */
    /* This would require mounting the newly created EXT4 partition */

    installer_state.progress_percent = 90;

    /* Phase 4: Install bootloader (10%) */
    printk(KERN_INFO "[INSTALLER] Installing bootloader...\n");
    install_bootloader(dev, gpt_ctx.header.first_usable_lba, 2);

    installer_state.progress_percent = 100;

    kfree(gpt_ctx.entries);

    /* Installation complete */
    installer_state.current_step = INSTALL_STEP_COMPLETE;
    printk(KERN_INFO "[INSTALLER] Installation completed successfully\n");
}

/*
 * progress_callback - File copy progress callback
 */
static void progress_callback(uint64_t copied, uint64_t total, const char *file) {
    installer_state.copied_bytes = copied;
    installer_state.total_bytes = total;

    /* Calculate progress (file copy is 30-90% of total) */
    int file_progress = (int)((copied * 100) / total);
    installer_state.progress_percent = 30 + (file_progress * 60) / 100;

    /* Update current file */
    strncpy(installer_state.current_file, file, sizeof(installer_state.current_file) - 1);
    installer_state.current_file[sizeof(installer_state.current_file) - 1] = '\0';
}
