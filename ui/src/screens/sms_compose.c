/*
 * StealthOS — SMS Compose Screen
 * T9 text input for composing encrypted messages.
 */

#include "../stealth_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"

typedef struct {
    framebuffer_t *fb;
    t9_input_t t9;
    char recipient[MAX_PHONE_NUMBER + 1];
    int recipient_len;
    bool editing_recipient;
} sms_compose_state_t;

/* Forward declaration */
extern const char *t9_mode_name(void);
extern void t9_backspace(t9_input_t *t9);
extern void t9_tick(t9_input_t *t9);


static void sms_compose_render(void *state)
{
    sms_compose_state_t *s = (sms_compose_state_t *)state;
    framebuffer_t *fb = s->fb;

    fb_clear(fb, COL_BG_PRIMARY);

    /* Title */
    fb_rect(fb, 0, 0, fb->width, 24, COL_BG_SECONDARY);
    fb_text_centered(fb, 5, "New Message", 14, COL_FG_PRIMARY);

    /* Recipient field */
    int field_y = 30;
    fb_text(fb, 8, field_y, "To:", 12, COL_FG_DIM);
    color_t rec_bg = s->editing_recipient ? COL_BG_SELECTED : COL_BG_CARD;
    fb_rect_rounded(fb, 36, field_y - 4, fb->width - 44, 22, 4, rec_bg);
    if (s->recipient_len > 0) {
        fb_text(fb, 42, field_y, s->recipient, 12, COL_FG_PRIMARY);
    } else {
        fb_text(fb, 42, field_y, "Enter number...", 12, COL_FG_DIM);
    }

    /* Message body */
    int body_y = 60;
    fb_text(fb, 8, body_y, "Message:", 12, COL_FG_DIM);
    fb_rect_rounded(fb, 8, body_y + 16, fb->width - 16, 140, 6, COL_BG_CARD);

    const char *text = t9_get_text(&s->t9);
    if (text[0]) {
        fb_text(fb, 14, body_y + 22, text, 13, COL_FG_PRIMARY);
    } else if (!s->editing_recipient) {
        fb_text(fb, 14, body_y + 22, "Type your message...", 13, COL_FG_DIM);
    }

    /* Input mode indicator */
    const char *mode = t9_mode_name();
    fb_rect_rounded(fb, fb->width - 50, body_y + 140, 40, 16, 3, COL_BG_SECONDARY);
    fb_text(fb, fb->width - 45, body_y + 142, mode, 10, COL_FG_DIM);

    /* Character count */
    char count[16];
    int len = strlen(text);
    snprintf(count, sizeof(count), "%d/160", len);
    color_t count_col = len > 160 ? COL_DANGER : COL_FG_DIM;
    fb_text(fb, 14, body_y + 142, count, 10, count_col);

    /* Encryption badge */
    fb_rect_rounded(fb, 30, 220, fb->width - 60, 20, 4,
                    COLOR_ARGB(0x30, 0x00, 0xD4, 0x7E));
    fb_text_centered(fb, 224, "End-to-end encrypted", 10, COL_ENCRYPTED);

    /* Soft keys */
    int soft_y = fb->height - 20;
    fb_rect(fb, 0, soft_y - 2, fb->width, 22, COL_BG_SECONDARY);
    fb_text(fb, 8, soft_y, "Send", 12, COL_ACCENT);
    fb_text(fb, fb->width - 36, soft_y, "Back", 12, COL_FG_DIM);

    fb_flip(fb);
}

static bool sms_compose_on_key(void *state, const key_event_t *event)
{
    sms_compose_state_t *s = (sms_compose_state_t *)state;

    if (event->type != KEY_EVENT_PRESS) return false;

    if (s->editing_recipient) {
        /* Number input for recipient */
        if (event->key >= KEY_0 && event->key <= KEY_9 &&
            s->recipient_len < MAX_PHONE_NUMBER) {
            s->recipient[s->recipient_len] = '0' + event->key;
            s->recipient_len++;
            s->recipient[s->recipient_len] = '\0';
            return true;
        }
        if (event->key == KEY_DOWN || event->key == KEY_CENTER) {
            s->editing_recipient = false;
            return true;
        }
        if (event->key == KEY_SOFT_RIGHT && s->recipient_len > 0) {
            s->recipient_len--;
            s->recipient[s->recipient_len] = '\0';
            return true;
        }
    } else {
        /* T9 text input for message body */
        if (event->key >= KEY_0 && event->key <= KEY_9) {
            t9_handle_key(&s->t9, event->key);
            return true;
        }
        if (event->key == KEY_STAR) {
            t9_handle_key(&s->t9, KEY_STAR);
            return true;
        }
        if (event->key == KEY_HASH) {
            t9_handle_key(&s->t9, KEY_HASH);  /* Mode switch */
            return true;
        }
        if (event->key == KEY_SOFT_RIGHT) {
            t9_backspace(&s->t9);
            return true;
        }
        if (event->key == KEY_UP) {
            s->editing_recipient = true;
            return true;
        }
        if (event->key == KEY_SOFT_LEFT || event->key == KEY_CENTER) {
            /* Send message */
            if (s->recipient_len > 0 && s->t9.length > 0) {
                const char *msg_text = t9_get_text(&s->t9);
                telephony_send_sms(s->recipient, msg_text);
                storage_message_add(s->recipient, msg_text, 1); /* 1 = outgoing */
                screen_manager_pop();
            }
            return true;
        }
    }

    return false;
}

static void sms_compose_on_enter(void *state)
{
    sms_compose_state_t *s = (sms_compose_state_t *)state;
    t9_clear(&s->t9);
    s->recipient_len = 0;
    s->recipient[0] = '\0';
    s->editing_recipient = true;
}

static void sms_compose_on_exit(void *state) { (void)state; }

screen_t *screen_sms_compose_create(framebuffer_t *fb)
{
    screen_t *scr = calloc(1, sizeof(screen_t));
    sms_compose_state_t *state = calloc(1, sizeof(sms_compose_state_t));

    state->fb = fb;
    t9_init(&state->t9);

    scr->id = SCREEN_SMS_COMPOSE;
    scr->name = "Compose";
    scr->on_enter = sms_compose_on_enter;
    scr->on_exit = sms_compose_on_exit;
    scr->render = sms_compose_render;
    scr->on_key = sms_compose_on_key;
    scr->state = state;

    return scr;
}
