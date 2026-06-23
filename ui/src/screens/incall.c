/*
 * StealthOS — In-Call Screen
 * Active call display with call duration, encryption status, and controls.
 */

#include "../stealth_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    framebuffer_t *fb;
    char number[32];
    const char *contact_name;
    uint32_t call_start_time;
    bool muted;
    bool speaker;
    bool connected;     /* false = ringing, true = connected */
} incall_state_t;

static void incall_render(void *state)
{
    incall_state_t *s = (incall_state_t *)state;
    framebuffer_t *fb = s->fb;

    /* Dark gradient background */
    fb_gradient_v(fb, 0, 0, fb->width, fb->height,
                  COL_BG_PRIMARY, COLOR_RGB(0x05, 0x0A, 0x15));

    /* ─── Encryption indicator at top ────────────────────────────────── */
    fb_rect_rounded(fb, (fb->width - 140) / 2, 12, 140, 20, 10,
                    COLOR_ARGB(0x40, 0x00, 0xD4, 0x7E));
    fb_text_centered(fb, 16, "Encrypted Call", 11, COL_ENCRYPTED);

    /* ─── Contact info ───────────────────────────────────────────────── */
    /* Avatar circle */
    fb_circle_filled(fb, fb->width / 2, 80, 30,
                     COLOR_RGB(0x5A, 0x9C, 0xFF));
    if (s->contact_name) {
        char initial[2] = { s->contact_name[0], '\0' };
        fb_text(fb, fb->width / 2 - 8, 68, initial, 24, COL_BG_PRIMARY);
    }

    /* Contact name or number */
    const char *display = s->contact_name ? s->contact_name : s->number;
    fb_text_centered(fb, 120, display, 20, COL_FG_PRIMARY);

    /* Status */
    const char *status = s->connected ? "Connected" : "Calling...";
    fb_text_centered(fb, 148, status, 13, COL_FG_SECONDARY);

    /* ─── Call duration ──────────────────────────────────────────────── */
    if (s->connected && s->call_start_time > 0) {
        uint32_t elapsed = (uint32_t)time(NULL) - s->call_start_time;
        int mins = elapsed / 60;
        int secs = elapsed % 60;
        char dur[16];
        snprintf(dur, sizeof(dur), "%02d:%02d", mins, secs);
        fb_text_centered(fb, 170, dur, 16, COL_FG_PRIMARY);
    }

    /* ─── Call controls ──────────────────────────────────────────────── */
    int ctrl_y = 210;
    int btn_size = 40;
    int gap = 30;
    int start_x = (fb->width - (3 * btn_size + 2 * gap)) / 2;

    /* Mute button */
    color_t mute_col = s->muted ? COL_WARNING : COL_BG_CARD;
    fb_circle_filled(fb, start_x + btn_size / 2, ctrl_y + btn_size / 2,
                     btn_size / 2, mute_col);
    fb_text(fb, start_x + 12, ctrl_y + 12, "M", 16,
            s->muted ? COL_BG_PRIMARY : COL_FG_PRIMARY);

    /* Speaker button */
    int spk_x = start_x + btn_size + gap;
    color_t spk_col = s->speaker ? COL_ACCENT : COL_BG_CARD;
    fb_circle_filled(fb, spk_x + btn_size / 2, ctrl_y + btn_size / 2,
                     btn_size / 2, spk_col);
    fb_text(fb, spk_x + 12, ctrl_y + 12, "S", 16,
            s->speaker ? COL_BG_PRIMARY : COL_FG_PRIMARY);

    /* Keypad button */
    int kp_x = spk_x + btn_size + gap;
    fb_circle_filled(fb, kp_x + btn_size / 2, ctrl_y + btn_size / 2,
                     btn_size / 2, COL_BG_CARD);
    fb_text(fb, kp_x + 12, ctrl_y + 12, "#", 16, COL_FG_PRIMARY);

    /* ─── End call button ────────────────────────────────────────────── */
    int end_w = 120;
    int end_h = 36;
    int end_x = (fb->width - end_w) / 2;
    int end_y = fb->height - 60;

    fb_rect_rounded(fb, end_x, end_y, end_w, end_h, 18, COL_CALL_RED);
    fb_text_centered(fb, end_y + 10, "End Call", 14, COL_FG_PRIMARY);

    fb_flip(fb);
}

static bool incall_on_key(void *state, const key_event_t *event)
{
    incall_state_t *s = (incall_state_t *)state;

    if (event->type != KEY_EVENT_PRESS) return false;

    switch (event->key) {
        case KEY_END:
        case KEY_SOFT_RIGHT:
            telephony_hangup();
            screen_manager_pop();
            return true;

        case KEY_1:
        case KEY_LEFT:
            s->muted = !s->muted;
            return true;

        case KEY_2:
        case KEY_CENTER:
            s->speaker = !s->speaker;
            return true;

        case KEY_CALL:
            if (!s->connected) {
                /* Answer incoming call */
                telephony_answer();
                s->connected = true;
                s->call_start_time = (uint32_t)time(NULL);
            }
            return true;

        default:
            break;
    }
    return false;
}

static void incall_on_enter(void *state)
{
    incall_state_t *s = (incall_state_t *)state;
    s->muted = false;
    s->speaker = false;
    s->connected = false;
    s->call_start_time = 0;
    s->contact_name = "Alice";  /* TODO: lookup from contacts */
}

static void incall_on_exit(void *state) { (void)state; }

screen_t *screen_incall_create(framebuffer_t *fb)
{
    screen_t *scr = calloc(1, sizeof(screen_t));
    incall_state_t *state = calloc(1, sizeof(incall_state_t));

    state->fb = fb;

    scr->id = SCREEN_INCALL;
    scr->name = "In Call";
    scr->on_enter = incall_on_enter;
    scr->on_exit = incall_on_exit;
    scr->render = incall_render;
    scr->on_key = incall_on_key;
    scr->state = state;

    return scr;
}
