/*
 * StealthOS — Settings Screen
 * System settings with encryption status, security options, and device info.
 */

#include "../stealth_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *label;
    const char *value;
    bool is_section;
} setting_item_t;

static const setting_item_t settings[] = {
    { "SECURITY",         NULL,                true  },
    { "Disk Encryption",  "AES-256-XTS (ON)",  false },
    { "Boot Integrity",   "dm-verity (OK)",    false },
    { "Comm Encryption",  "Signal Protocol",   false },
    { "PIN Lock",         "Enabled",           false },
    { "NETWORK",          NULL,                true  },
    { "SIM Card",         "Active",            false },
    { "Signal Strength",  "4/5 bars",          false },
    { "Data",             "Disabled",          false },
    { "SYSTEM",           NULL,                true  },
    { "StealthOS",        "v0.1.0",            false },
    { "Kernel",           "5.15.150-stealth",  false },
    { "Uptime",           "2d 14h 32m",        false },
    { "Storage",          "87MB / 128MB",      false },
};

#define SETTING_COUNT (sizeof(settings) / sizeof(settings[0]))

typedef struct {
    framebuffer_t *fb;
    int selected;
    int scroll_offset;
} settings_state_t;

static void settings_render(void *state)
{
    settings_state_t *s = (settings_state_t *)state;
    framebuffer_t *fb = s->fb;

    fb_clear(fb, COL_BG_PRIMARY);

    /* Title */
    fb_rect(fb, 0, 0, fb->width, 24, COL_BG_SECONDARY);
    fb_text_centered(fb, 5, "Settings", 14, COL_FG_PRIMARY);

    /* Settings list */
    int y = 28;
    int item_count = 0;

    for (int i = s->scroll_offset; i < (int)SETTING_COUNT; i++) {
        if (y + 30 > fb->height - 24) break;

        const setting_item_t *item = &settings[i];

        if (item->is_section) {
            /* Section header */
            y += 6;
            fb_text(fb, 12, y + 4, item->label, 10, COL_ACCENT);
            fb_rect(fb, 12, y + 18, fb->width - 24, 1, COL_BG_CARD);
            y += 22;
        } else {
            /* Regular setting */
            if (item_count == s->selected) {
                fb_rect(fb, 0, y, fb->width, 28, COL_BG_SELECTED);
            }

            fb_text(fb, 16, y + 6, item->label, 13, COL_FG_PRIMARY);

            if (item->value) {
                int val_w = fb_text_width(item->value, 11);
                color_t val_col = COL_FG_DIM;

                /* Color-code security status */
                if (strstr(item->value, "ON") || strstr(item->value, "OK") ||
                    strstr(item->value, "Enabled") || strstr(item->value, "Active")) {
                    val_col = COL_ACCENT;
                }
                if (strstr(item->value, "Disabled")) {
                    val_col = COL_DANGER;
                }

                fb_text(fb, fb->width - val_w - 12, y + 8,
                        item->value, 11, val_col);
            }

            y += 30;
            item_count++;
        }
    }

    /* Soft keys */
    int soft_y = fb->height - 20;
    fb_rect(fb, 0, soft_y - 2, fb->width, 22, COL_BG_SECONDARY);
    fb_text(fb, 8, soft_y, "Select", 12, COL_FG_DIM);
    fb_text(fb, fb->width - 36, soft_y, "Back", 12, COL_FG_DIM);

    fb_flip(fb);
}

static bool settings_on_key(void *state, const key_event_t *event)
{
    settings_state_t *s = (settings_state_t *)state;

    if (event->type != KEY_EVENT_PRESS) return false;

    /* Count non-section items */
    int max_sel = 0;
    for (int i = 0; i < (int)SETTING_COUNT; i++) {
        if (!settings[i].is_section) max_sel++;
    }

    switch (event->key) {
        case KEY_UP:
            if (s->selected > 0) s->selected--;
            return true;
        case KEY_DOWN:
            if (s->selected < max_sel - 1) s->selected++;
            return true;
        default:
            break;
    }
    return false;
}

static void settings_on_enter(void *state) { (void)state; }
static void settings_on_exit(void *state) { (void)state; }

screen_t *screen_settings_create(framebuffer_t *fb)
{
    screen_t *scr = calloc(1, sizeof(screen_t));
    settings_state_t *state = calloc(1, sizeof(settings_state_t));

    state->fb = fb;
    state->selected = 0;

    scr->id = SCREEN_SETTINGS;
    scr->name = "Settings";
    scr->on_enter = settings_on_enter;
    scr->on_exit = settings_on_exit;
    scr->render = settings_render;
    scr->on_key = settings_on_key;
    scr->state = state;

    return scr;
}
