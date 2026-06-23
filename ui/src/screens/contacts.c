/*
 * StealthOS — Contacts Screen
 * Scrollable alphabetical contact list.
 */

#include "../stealth_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage.h"

static Contact g_contacts[64];
static int     g_contact_count = 0;

typedef struct {
    framebuffer_t *fb;
    int selected;
} contacts_state_t;

static void contacts_render(void *state)
{
    contacts_state_t *s = (contacts_state_t *)state;
    framebuffer_t *fb = s->fb;

    fb_clear(fb, COL_BG_PRIMARY);

    /* Title */
    fb_rect(fb, 0, 0, fb->width, 24, COL_BG_SECONDARY);
    fb_text_centered(fb, 5, "Contacts", 14, COL_FG_PRIMARY);

    /* Contact list */
    int item_h = 42;
    int list_y = 28;

    for (int i = 0; i < g_contact_count; i++) {
        int y = list_y + i * item_h;
        if (y + item_h > fb->height - 24) break;

        /* Selection highlight */
        if (i == s->selected) {
            fb_rect(fb, 0, y, fb->width, item_h - 2, COL_BG_SELECTED);
        }

        /* Avatar circle (first letter) */
        char initial[2] = { g_contacts[i].name[0], '\0' };
        color_t avatar_colors[] = {
            COLOR_RGB(0x5A, 0x9C, 0xFF),
            COLOR_RGB(0xFF, 0x7E, 0x5F),
            COL_ACCENT,
            COLOR_RGB(0xBB, 0x86, 0xFC),
            COLOR_RGB(0xFF, 0xCC, 0x02),
            COLOR_RGB(0xFF, 0x5C, 0x8D),
        };
        fb_circle_filled(fb, 24, y + item_h / 2 - 1, 14,
                        avatar_colors[i % 6]);
        fb_text(fb, 18, y + 8, initial, 16, COL_BG_PRIMARY);

        /* Name */
        fb_text(fb, 48, y + 6, g_contacts[i].name, 14, COL_FG_PRIMARY);

        /* Number */
        fb_text(fb, 48, y + 24, g_contacts[i].number, 11, COL_FG_DIM);
    }

    /* Soft keys */
    int soft_y = fb->height - 20;
    fb_rect(fb, 0, soft_y - 2, fb->width, 22, COL_BG_SECONDARY);
    fb_text(fb, 8, soft_y, "Call", 12, COL_CALL_GREEN);
    fb_text(fb, fb->width - 36, soft_y, "Back", 12, COL_FG_DIM);

    fb_flip(fb);
}

static bool contacts_on_key(void *state, const key_event_t *event)
{
    contacts_state_t *s = (contacts_state_t *)state;

    if (event->type != KEY_EVENT_PRESS) return false;

    switch (event->key) {
        case KEY_UP:
            if (s->selected > 0) s->selected--;
            return true;
        case KEY_DOWN:
            if (s->selected < g_contact_count - 1) s->selected++;
            return true;
        case KEY_CALL:
        case KEY_CENTER:
        case KEY_SOFT_LEFT:
            if (g_contact_count > 0) {
                telephony_dial(g_contacts[s->selected].number);
            }
            return true;
        default:
            break;
    }
    return false;
}

static void contacts_on_enter(void *state) {
    (void)state;
    g_contact_count = storage_contact_list(g_contacts, 64);
}
static void contacts_on_exit(void *state) { (void)state; }

screen_t *screen_contacts_create(framebuffer_t *fb)
{
    screen_t *scr = calloc(1, sizeof(screen_t));
    contacts_state_t *state = calloc(1, sizeof(contacts_state_t));

    state->fb = fb;
    state->selected = 0;

    scr->id = SCREEN_CONTACTS;
    scr->name = "Contacts";
    scr->on_enter = contacts_on_enter;
    scr->on_exit = contacts_on_exit;
    scr->render = contacts_render;
    scr->on_key = contacts_on_key;
    scr->state = state;

    return scr;
}
