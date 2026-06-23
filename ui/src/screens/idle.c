/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — Idle / Lock Screen
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Shows time, date, encryption status, and signal strength.
 * Press any key to go to PIN entry (if locked) or home screen.
 *
 * Layout (240x320):
 * ┌──────────────────────┐
 * │ ▮▮▮▮  StealthOS  🔒 │  ← Status bar (signal, carrier, lock)
 * │                      │
 * │                      │
 * │       14:32          │  ← Large time
 * │    Mon, Jun 23       │  ← Date
 * │                      │
 * │                      │
 * │   🔐 Encrypted       │  ← Encryption badge
 * │                      │
 * │                      │
 * │ Menu          Unlock │  ← Soft keys
 * └──────────────────────┘
 */

#include "../stealth_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ─── Idle Screen State ──────────────────────────────────────────────────── */

typedef struct {
    framebuffer_t *fb;
    bool locked;
    int blink_counter;      /* For cursor blink / animation */
} idle_state_t;

/* ─── Status Bar Rendering ───────────────────────────────────────────────── */

static void render_status_bar(framebuffer_t *fb)
{
    /* Background */
    fb_rect(fb, 0, 0, fb->width, 20, COL_BG_SECONDARY);

    /* Signal bars (left side) */
    int bar_x = 4;
    int bar_heights[] = {4, 7, 10, 13, 16};
    int signal_strength = 4;  /* TODO: get from modem */

    for (int i = 0; i < 5; i++) {
        color_t col = (i < signal_strength) ? COL_SIGNAL_BAR : COL_FG_DIM;
        fb_rect(fb, bar_x + i * 5, 18 - bar_heights[i],
                3, bar_heights[i], col);
    }

    /* Carrier name */
    fb_text(fb, 32, 3, "StealthOS", 12, COL_FG_SECONDARY);

    /* Lock icon (right side) */
    fb_text(fb, fb->width - 20, 3, "E", 12, COL_ENCRYPTED);

    /* Battery (far right) — simple rectangle */
    int batt_x = fb->width - 40;
    fb_rect(fb, batt_x, 5, 16, 10, COL_FG_DIM);       /* Outline */
    fb_rect(fb, batt_x + 16, 8, 2, 4, COL_FG_DIM);    /* Nub */
    fb_rect(fb, batt_x + 2, 7, 10, 6, COL_ACCENT);    /* Fill */
}

/* ─── Screen Callbacks ───────────────────────────────────────────────────── */

static void idle_on_enter(void *state)
{
    idle_state_t *s = (idle_state_t *)state;
    s->locked = true;
    s->blink_counter = 0;
}

static void idle_on_exit(void *state)
{
    (void)state;
}

static void idle_render(void *state)
{
    idle_state_t *s = (idle_state_t *)state;
    framebuffer_t *fb = s->fb;

    /* Clear screen */
    fb_clear(fb, COL_BG_PRIMARY);

    /* Status bar */
    render_status_bar(fb);

    /* ─── Time (large, centered) ─────────────────────────────────────── */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char time_str[8];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", t->tm_hour, t->tm_min);
    fb_text_centered(fb, 100, time_str, 48, COL_FG_PRIMARY);

    /* ─── Date ───────────────────────────────────────────────────────── */
    const char *days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                            "Jul","Aug","Sep","Oct","Nov","Dec"};
    char date_str[32];
    snprintf(date_str, sizeof(date_str), "%s, %s %d",
             days[t->tm_wday], months[t->tm_mon], t->tm_mday);
    fb_text_centered(fb, 155, date_str, 16, COL_FG_SECONDARY);

    /* ─── Encryption Status Badge ────────────────────────────────────── */
    int badge_w = 120;
    int badge_h = 28;
    int badge_x = (fb->width - badge_w) / 2;
    int badge_y = 200;

    fb_rect_rounded(fb, badge_x, badge_y, badge_w, badge_h, 6,
                    COLOR_ARGB(0x40, 0x00, 0xD4, 0x7E));
    fb_text_centered(fb, badge_y + 7, "Encrypted", 14, COL_ENCRYPTED);

    /* ─── Soft Key Labels ────────────────────────────────────────────── */
    int soft_y = fb->height - 22;
    fb_rect(fb, 0, soft_y - 2, fb->width, 24, COL_BG_SECONDARY);
    fb_text(fb, 8, soft_y, "Menu", 14, COL_FG_SECONDARY);

    const char *right_label = s->locked ? "Unlock" : "Lock";
    int right_w = fb_text_width(right_label, 14);
    fb_text(fb, fb->width - right_w - 8, soft_y, right_label,
            14, COL_FG_SECONDARY);

    /* ─── Flip to display ────────────────────────────────────────────── */
    fb_flip(fb);

    s->blink_counter++;
}

static bool idle_on_key(void *state, const key_event_t *event)
{
    idle_state_t *s = (idle_state_t *)state;
    (void)s;

    if (event->type != KEY_EVENT_PRESS) return false;

    switch (event->key) {
        case KEY_SOFT_LEFT:
            /* Go to home/menu */
            screen_manager_push(SCREEN_HOME);
            return true;

        case KEY_SOFT_RIGHT:
        case KEY_CENTER:
            /* Unlock / go to home */
            screen_manager_push(SCREEN_HOME);
            return true;

        case KEY_CALL:
            /* Quick dial */
            screen_manager_push(SCREEN_DIALER);
            return true;

        default:
            break;
    }

    return false;
}

/* ─── Screen Factory ─────────────────────────────────────────────────────── */

screen_t *screen_idle_create(framebuffer_t *fb)
{
    screen_t *scr = calloc(1, sizeof(screen_t));
    idle_state_t *state = calloc(1, sizeof(idle_state_t));

    state->fb = fb;
    state->locked = true;

    scr->id = SCREEN_IDLE;
    scr->name = "Idle";
    scr->on_enter = idle_on_enter;
    scr->on_exit = idle_on_exit;
    scr->render = idle_render;
    scr->on_key = idle_on_key;
    scr->state = state;

    return scr;
}
