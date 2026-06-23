/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — SMS List Screen
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Scrollable list of SMS conversations.
 * Each row shows: contact name/number, last message preview, timestamp.
 * Navigate with up/down, open with center, compose with soft-left.
 */

#include "../stealth_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage.h"

static Message g_messages[64];
static int g_message_count = 0;

/* ─── State ──────────────────────────────────────────────────────────────── */

typedef struct {
    framebuffer_t *fb;
    int selected;
    int scroll_offset;
} sms_list_state_t;

/* ─── Rendering ──────────────────────────────────────────────────────────── */

static void sms_list_render(void *state)
{
    sms_list_state_t *s = (sms_list_state_t *)state;
    framebuffer_t *fb = s->fb;

    fb_clear(fb, COL_BG_PRIMARY);

    /* Title bar */
    fb_rect(fb, 0, 0, fb->width, 24, COL_BG_SECONDARY);
    fb_text_centered(fb, 5, "Messages", 14, COL_FG_PRIMARY);

    /* Message list */
    int item_h = 52;
    int list_y = 28;
    int visible_items = (fb->height - 50) / item_h;

    for (int i = 0; i < g_message_count && i < visible_items; i++) {
        int idx = i + s->scroll_offset;
        if (idx >= g_message_count) break;

        int y = list_y + i * item_h;
        const Message *msg = &g_messages[idx];

        /* Background highlight for selected */
        if (idx == s->selected) {
            fb_rect(fb, 0, y, fb->width, item_h - 2, COL_BG_SELECTED);
        }

        /* Unread indicator - stubbed */
        bool unread = false;
        if (unread) {
            fb_circle_filled(fb, 10, y + item_h / 2, 4, COL_ACCENT);
        }

        /* Contact name or number */
        color_t name_col = unread ? COL_FG_PRIMARY : COL_FG_SECONDARY;
        fb_text(fb, 22, y + 6, msg->number, 14, name_col);

        /* Timestamp (right-aligned) */
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%ld", msg->timestamp);
        int time_w = fb_text_width(time_str, 10);
        fb_text(fb, fb->width - time_w - 8, y + 8, time_str, 10, COL_FG_DIM);

        /* Preview text */
        fb_text(fb, 22, y + 26, msg->body, 11, COL_FG_DIM);

        /* Separator line */
        fb_rect(fb, 22, y + item_h - 3, fb->width - 30, 1,
                COLOR_ARGB(0x20, 0xFF, 0xFF, 0xFF));
    }

    /* Soft keys */
    int soft_y = fb->height - 20;
    fb_rect(fb, 0, soft_y - 2, fb->width, 22, COL_BG_SECONDARY);
    fb_text(fb, 8, soft_y, "New", 12, COL_ACCENT);
    fb_text(fb, fb->width - 36, soft_y, "Back", 12, COL_FG_DIM);

    fb_flip(fb);
}

/* ─── Input ──────────────────────────────────────────────────────────────── */

static bool sms_list_on_key(void *state, const key_event_t *event)
{
    sms_list_state_t *s = (sms_list_state_t *)state;

    if (event->type != KEY_EVENT_PRESS) return false;

    switch (event->key) {
        case KEY_UP:
            if (s->selected > 0) s->selected--;
            return true;

        case KEY_DOWN:
            if (s->selected < g_message_count - 1) s->selected++;
            return true;

        case KEY_CENTER:
            /* TODO: Open message view */
            return true;

        case KEY_SOFT_LEFT:
            screen_manager_push(SCREEN_SMS_COMPOSE);
            return true;

        default:
            break;
    }

    return false;
}

static void sms_list_on_enter(void *state) {
    (void)state;
    g_message_count = storage_thread_list(g_messages, 64);
}
static void sms_list_on_exit(void *state) { (void)state; }

/* ─── Factory ────────────────────────────────────────────────────────────── */

screen_t *screen_sms_list_create(framebuffer_t *fb)
{
    screen_t *scr = calloc(1, sizeof(screen_t));
    sms_list_state_t *state = calloc(1, sizeof(sms_list_state_t));

    state->fb = fb;
    state->selected = 0;
    state->scroll_offset = 0;

    scr->id = SCREEN_SMS_LIST;
    scr->name = "Messages";
    scr->on_enter = sms_list_on_enter;
    scr->on_exit = sms_list_on_exit;
    scr->render = sms_list_render;
    scr->on_key = sms_list_on_key;
    scr->state = state;

    return scr;
}
