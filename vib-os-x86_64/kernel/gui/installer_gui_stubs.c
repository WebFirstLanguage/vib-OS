/*
 * GUI Stubs for installer in x86_64 build
 * Provides minimal compatibility layer
 */

#include "gui.h"
#include "types.h"

/* Stub window for installer */
static struct {
    int id;
    char title[64];
    int x, y;
    int width, height;
    bool visible;
    void (*on_draw)(void *win);
    void (*on_key)(void *win, int key);
    void (*on_mouse)(void *win, int x, int y, int buttons);
    void *userdata;
} installer_window;

/* Create installer window (stub) */
void *gui_create_window(const char *title, int x, int y, int w, int h) {
    installer_window.x = x;
    installer_window.y = y;
    installer_window.width = w;
    installer_window.height = h;
    installer_window.visible = true;

    int len = 0;
    while (title[len] && len < 63) {
        installer_window.title[len] = title[len];
        len++;
    }
    installer_window.title[len] = '\0';

    return &installer_window;
}

/* Draw rectangle - use compositor's internal function */
void gui_draw_rect(int x, int y, int w, int h, uint32_t color) {
    /* Stub - installer drawing will be handled by compositor */
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

/* gui_draw_string already exists in compositor.c */
