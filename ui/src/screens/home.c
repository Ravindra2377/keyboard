/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — Home / Main Menu Screen
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Grid menu with icons for the core phone functions.
 * Navigated with D-pad (up/down/left/right + center to select).
 *
 * Layout (240x320):
 * ┌──────────────────────┐
 * │ ▮▮▮▮  StealthOS  🔒 │
 * │                      │
 * │  ┌──────┐ ┌──────┐  │
 * │  │  📞  │ │  ✉   │  │
 * │  │ Phone│ │ SMS  │  │
 * │  └──────┘ └──────┘  │
 * │  ┌──────┐ ┌──────┐  │
 * │  │  👤  │ │  ⚙   │  │
 * │  │Contac│ │Settin│  │
 * │  └──────┘ └──────┘  │
 * │                      │
 * │ Select        Back  │
 * └──────────────────────┘
 */

#include "../stealth_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─── Menu Items ─────────────────────────────────────────────────────────── */

typedef struct {
    const char *label;
    const char *icon;      /* Simple text icon for now */
    screen_id_t target;
    color_t accent;
} menu_item_t;

static const menu_item_t menu_items[] = {
    { "Phone",    "P", SCREEN_DIALER,      COL_CALL_GREEN },
    { "Messages", "M", SCREEN_SMS_LIST,    COL_ACCENT     },
    { "Contacts", "C", SCREEN_CONTACTS,    COLOR_RGB(0x5A, 0x9C, 0xFF) },
    { "Settings", "S", SCREEN_SETTINGS,    COL_FG_SECONDARY },
};

#define MENU_COLS 2
#define MENU_ROWS 2
#define MENU_COUNT (sizeof(menu_items) / sizeof(menu_items[0]))

/* ─── State ──────────────────────────────────────────────────────────────── */

typedef struct {
    framebuffer_t *fb;
    int selected;          /* Currently selected menu index */
} home_state_t;

/* ─── Rendering ──────────────────────────────────────────────────────────── */

static void home_render(void *state)
{
    home_state_t *s = (home_state_t *)state;
    framebuffer_t *fb = s->fb;

    fb_clear(fb, COL_BG_PRIMARY);

    /* ─── Title ──────────────────────────────────────────────────────── */
    fb_rect(fb, 0, 0, fb->width, 20, COL_BG_SECONDARY);
    fb_text_centered(fb, 3, "StealthOS", 14, COL_FG_SECONDARY);

    /* ─── Menu Grid ──────────────────────────────────────────────────── */
    int grid_x = 20;
    int grid_y = 50;
    int item_w = 92;
    int item_h = 90;
    int gap = 16;

    for (int i = 0; i < (int)MENU_COUNT; i++) {
        int row = i / MENU_COLS;
        int col = i % MENU_COLS;
        int x = grid_x + col * (item_w + gap);
        int y = grid_y + row * (item_h + gap);

        /* Item background */
        color_t bg = (i == s->selected) ? COL_BG_SELECTED : COL_BG_CARD;
        fb_rect_rounded(fb, x, y, item_w, item_h, 10, bg);

        /* Selection indicator — accent border */
        if (i == s->selected) {
            /* Top accent line */
            fb_rect(fb, x + 10, y, item_w - 20, 2, menu_items[i].accent);
        }

        /* Icon (large centered letter for now) */
        int icon_x = x + item_w / 2 -
                     fb_text_width(menu_items[i].icon, 32) / 2;
        fb_text(fb, icon_x, y + 15, menu_items[i].icon, 32,
                menu_items[i].accent);

        /* Label */
        int label_x = x + item_w / 2 -
                      fb_text_width(menu_items[i].label, 12) / 2;
        color_t label_col = (i == s->selected)
                            ? COL_FG_PRIMARY : COL_FG_SECONDARY;
        fb_text(fb, label_x, y + 65, menu_items[i].label, 12, label_col);
    }

    /* ─── Encryption Status ──────────────────────────────────────────── */
    int status_y = grid_y + 2 * (item_h + gap) + 10;
    fb_rect_rounded(fb, 30, status_y, fb->width - 60, 24, 4,
                    COLOR_ARGB(0x30, 0x00, 0xD4, 0x7E));
    fb_text_centered(fb, status_y + 5, "All communications encrypted",
                     10, COL_ENCRYPTED);

    /* ─── Soft Keys ──────────────────────────────────────────────────── */
    int soft_y = fb->height - 22;
    fb_rect(fb, 0, soft_y - 2, fb->width, 24, COL_BG_SECONDARY);
    fb_text(fb, 8, soft_y, "Select", 14, COL_ACCENT);
    fb_text(fb, fb->width - 48, soft_y, "Back", 14, COL_FG_DIM);

    fb_flip(fb);
}

/* ─── Input ──────────────────────────────────────────────────────────────── */

static bool home_on_key(void *state, const key_event_t *event)
{
    home_state_t *s = (home_state_t *)state;

    if (event->type != KEY_EVENT_PRESS) return false;

    switch (event->key) {
        case KEY_UP:
            if (s->selected >= MENU_COLS)
                s->selected -= MENU_COLS;
            return true;

        case KEY_DOWN:
            if (s->selected + MENU_COLS < (int)MENU_COUNT)
                s->selected += MENU_COLS;
            return true;

        case KEY_LEFT:
            if (s->selected % MENU_COLS > 0)
                s->selected--;
            return true;

        case KEY_RIGHT:
            if (s->selected % MENU_COLS < MENU_COLS - 1 &&
                s->selected + 1 < (int)MENU_COUNT)
                s->selected++;
            return true;

        case KEY_CENTER:
        case KEY_SOFT_LEFT:
            screen_manager_push(menu_items[s->selected].target);
            return true;

        case KEY_CALL:
            /* Quick access to dialer */
            screen_manager_push(SCREEN_DIALER);
            return true;

        /* Number shortcuts */
        case KEY_1: screen_manager_push(SCREEN_DIALER); return true;
        case KEY_2: screen_manager_push(SCREEN_SMS_LIST); return true;
        case KEY_3: screen_manager_push(SCREEN_CONTACTS); return true;
        case KEY_4: screen_manager_push(SCREEN_SETTINGS); return true;

        default:
            break;
    }

    return false;
}

/* ─── Lifecycle ──────────────────────────────────────────────────────────── */

static void home_on_enter(void *state)
{
    home_state_t *s = (home_state_t *)state;
    /* Keep selection persistent across visits */
    (void)s;
}

static void home_on_exit(void *state) { (void)state; }

/* ─── Factory ────────────────────────────────────────────────────────────── */

screen_t *screen_home_create(framebuffer_t *fb)
{
    screen_t *scr = calloc(1, sizeof(screen_t));
    home_state_t *state = calloc(1, sizeof(home_state_t));

    state->fb = fb;
    state->selected = 0;

    scr->id = SCREEN_HOME;
    scr->name = "Home";
    scr->on_enter = home_on_enter;
    scr->on_exit = home_on_exit;
    scr->render = home_render;
    scr->on_key = home_on_key;
    scr->state = state;

    return scr;
}
