/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — Phone Dialer Screen
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Classic phone dialer with large number display and call button.
 *
 * Layout (240x320):
 * ┌──────────────────────┐
 * │ ▮▮▮▮  StealthOS  🔒 │  ← Status bar
 * │                      │
 * │  +1 555 012 3456     │  ← Number display (large)
 * │                      │
 * │  ┌─────┬─────┬─────┐ │
 * │  │  1  │  2  │  3  │ │
 * │  │     │ ABC │ DEF │ │
 * │  ├─────┼─────┼─────┤ │
 * │  │  4  │  5  │  6  │ │
 * │  │ GHI │ JKL │ MNO │ │
 * │  ├─────┼─────┼─────┤ │
 * │  │  7  │  8  │  9  │ │
 * │  │PQRS │ TUV │WXYZ │ │
 * │  ├─────┼─────┼─────┤ │
 * │  │  *  │  0  │  #  │ │
 * │  │     │  +  │     │ │
 * │  └─────┴─────┴─────┘ │
 * │ Save    [📞]    Back │  ← Soft keys + call button
 * └──────────────────────┘
 */

#include "../stealth_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─── Dialer State ───────────────────────────────────────────────────────── */


typedef struct {
    framebuffer_t *fb;
    char number[MAX_PHONE_NUMBER + 1];
    int num_len;
    int cursor_blink;
} dialer_state_t;

/* ─── Keypad Visual Layout ───────────────────────────────────────────────── */

static const char *key_labels[12] = {
    "1", "2", "3",
    "4", "5", "6",
    "7", "8", "9",
    "*", "0", "#"
};

static const char *key_sublabels[12] = {
    "",     "ABC",  "DEF",
    "GHI",  "JKL",  "MNO",
    "PQRS", "TUV",  "WXYZ",
    "",     "+",    ""
};

/* ─── Rendering ──────────────────────────────────────────────────────────── */

static void render_keypad_grid(framebuffer_t *fb, int start_y)
{
    int pad_x = 12;
    int key_w = 68;
    int key_h = 38;
    int gap = 4;

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            int idx = row * 3 + col;
            int x = pad_x + col * (key_w + gap);
            int y = start_y + row * (key_h + gap);

            /* Key background */
            fb_rect_rounded(fb, x, y, key_w, key_h, 6, COL_BG_CARD);

            /* Key number */
            int text_x = x + key_w / 2 - fb_text_width(key_labels[idx], 18) / 2;
            fb_text(fb, text_x, y + 4, key_labels[idx], 18, COL_FG_PRIMARY);

            /* Sub-label (letter hints) */
            if (key_sublabels[idx][0]) {
                int sub_x = x + key_w / 2 -
                           fb_text_width(key_sublabels[idx], 8) / 2;
                fb_text(fb, sub_x, y + 24, key_sublabels[idx],
                        8, COL_FG_DIM);
            }
        }
    }
}

static void dialer_render(void *state)
{
    dialer_state_t *s = (dialer_state_t *)state;
    framebuffer_t *fb = s->fb;

    fb_clear(fb, COL_BG_PRIMARY);

    /* ─── Number Display ─────────────────────────────────────────────── */
    fb_rect(fb, 0, 22, fb->width, 50, COL_BG_SECONDARY);

    if (s->num_len > 0) {
        /* Format number with spaces for readability */
        char formatted[40];
        int fi = 0;
        for (int i = 0; i < s->num_len && fi < 38; i++) {
            formatted[fi++] = s->number[i];
            /* Add space every 3 digits (phone number formatting) */
            if ((s->num_len - i - 1) % 3 == 0 && i < s->num_len - 1) {
                formatted[fi++] = ' ';
            }
        }
        formatted[fi] = '\0';

        fb_text_centered(fb, 32, formatted, 24, COL_FG_PRIMARY);
    } else {
        fb_text_centered(fb, 38, "Enter number", 16, COL_FG_DIM);
    }

    /* ─── Keypad Grid ────────────────────────────────────────────────── */
    render_keypad_grid(fb, 80);

    /* ─── Call Button ────────────────────────────────────────────────── */
    int call_y = fb->height - 50;
    int call_w = 80;
    int call_x = (fb->width - call_w) / 2;

    color_t call_color = (s->num_len > 0) ? COL_CALL_GREEN : COL_FG_DIM;
    fb_rect_rounded(fb, call_x, call_y, call_w, 30, 15, call_color);
    fb_text_centered(fb, call_y + 7, "CALL", 14, COL_BG_PRIMARY);

    /* ─── Soft Keys ──────────────────────────────────────────────────── */
    int soft_y = fb->height - 16;
    fb_text(fb, 8, soft_y, "Save", 12, COL_FG_DIM);
    fb_text(fb, fb->width - 40, soft_y, "Clear", 12, COL_FG_DIM);

    fb_flip(fb);
    s->cursor_blink++;
}

/* ─── Input Handling ─────────────────────────────────────────────────────── */

static bool dialer_on_key(void *state, const key_event_t *event)
{
    dialer_state_t *s = (dialer_state_t *)state;

    if (event->type != KEY_EVENT_PRESS) return false;

    switch (event->key) {
        case KEY_0: case KEY_1: case KEY_2: case KEY_3:
        case KEY_4: case KEY_5: case KEY_6: case KEY_7:
        case KEY_8: case KEY_9:
            if (s->num_len < MAX_PHONE_NUMBER) {
                s->number[s->num_len] = '0' + event->key;
                s->num_len++;
                s->number[s->num_len] = '\0';
            }
            return true;

        case KEY_STAR:
            if (s->num_len < MAX_PHONE_NUMBER) {
                s->number[s->num_len] = '*';
                s->num_len++;
                s->number[s->num_len] = '\0';
            }
            return true;

        case KEY_HASH:
            if (s->num_len < MAX_PHONE_NUMBER) {
                s->number[s->num_len] = '#';
                s->num_len++;
                s->number[s->num_len] = '\0';
            }
            return true;

        case KEY_CALL:
        case KEY_CENTER:
            if (s->num_len > 0) {
                /* Initiate call via oFono */
                telephony_dial(s->number);
                screen_manager_push(SCREEN_INCALL);
            }
            return true;

        case KEY_SOFT_RIGHT:
            /* Clear last digit */
            if (s->num_len > 0) {
                s->num_len--;
                s->number[s->num_len] = '\0';
            }
            return true;

        case KEY_END:
        case KEY_BACK:
            screen_manager_pop();
            return true;

        default:
            break;
    }

    return false;
}

/* ─── Lifecycle ──────────────────────────────────────────────────────────── */

static void dialer_on_enter(void *state)
{
    dialer_state_t *s = (dialer_state_t *)state;
    /* Don't clear number — allow re-entering dialer with number intact */
    s->cursor_blink = 0;
}

static void dialer_on_exit(void *state)
{
    (void)state;
}

/* ─── Factory ────────────────────────────────────────────────────────────── */

screen_t *screen_dialer_create(framebuffer_t *fb)
{
    screen_t *scr = calloc(1, sizeof(screen_t));
    dialer_state_t *state = calloc(1, sizeof(dialer_state_t));

    state->fb = fb;
    state->num_len = 0;
    memset(state->number, 0, sizeof(state->number));

    scr->id = SCREEN_DIALER;
    scr->name = "Dialer";
    scr->on_enter = dialer_on_enter;
    scr->on_exit = dialer_on_exit;
    scr->render = dialer_render;
    scr->on_key = dialer_on_key;
    scr->state = state;

    return scr;
}
