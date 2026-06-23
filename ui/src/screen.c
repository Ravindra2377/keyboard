/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — Screen Manager
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Manages a stack of screens (like a mobile navigation stack).
 * Screens can be pushed, popped, or replaced. Each screen gets
 * lifecycle callbacks (enter/exit) and handles its own rendering
 * and input.
 */

#include "stealth_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─── Screen Stack ───────────────────────────────────────────────────────── */

#define MAX_SCREEN_DEPTH 8

static struct {
    framebuffer_t *fb;
    screen_t *screens[SCREEN_COUNT];     /* Registered screens */
    screen_id_t stack[MAX_SCREEN_DEPTH]; /* Navigation stack */
    int stack_depth;
} manager;

/* ─── Screen Registration (forward declarations) ─────────────────────────── */

/* These are defined in screens/ */
extern screen_t *screen_idle_create(framebuffer_t *fb);
extern screen_t *screen_dialer_create(framebuffer_t *fb);
extern screen_t *screen_home_create(framebuffer_t *fb);
extern screen_t *screen_sms_list_create(framebuffer_t *fb);
extern screen_t *screen_sms_compose_create(framebuffer_t *fb);
extern screen_t *screen_contacts_create(framebuffer_t *fb);
extern screen_t *screen_incall_create(framebuffer_t *fb);
extern screen_t *screen_settings_create(framebuffer_t *fb);

/* ─── Init ───────────────────────────────────────────────────────────────── */

void screen_manager_init(framebuffer_t *fb)
{
    memset(&manager, 0, sizeof(manager));
    manager.fb = fb;
    manager.stack_depth = 0;

    /* Register all screens */
    manager.screens[SCREEN_IDLE]        = screen_idle_create(fb);
    manager.screens[SCREEN_DIALER]      = screen_dialer_create(fb);
    manager.screens[SCREEN_HOME]        = screen_home_create(fb);
    manager.screens[SCREEN_SMS_LIST]    = screen_sms_list_create(fb);
    manager.screens[SCREEN_SMS_COMPOSE] = screen_sms_compose_create(fb);
    manager.screens[SCREEN_CONTACTS]    = screen_contacts_create(fb);
    manager.screens[SCREEN_INCALL]      = screen_incall_create(fb);
    manager.screens[SCREEN_SETTINGS]    = screen_settings_create(fb);

    fprintf(stderr, "[screen] Manager initialized with %d screens\n",
            SCREEN_COUNT);
}

/* ─── Navigation ─────────────────────────────────────────────────────────── */

void screen_manager_push(screen_id_t id)
{
    if (manager.stack_depth >= MAX_SCREEN_DEPTH) {
        fprintf(stderr, "[screen] Stack overflow! Max depth %d\n",
                MAX_SCREEN_DEPTH);
        return;
    }

    if (id >= SCREEN_COUNT || !manager.screens[id]) {
        fprintf(stderr, "[screen] Invalid screen ID: %d\n", id);
        return;
    }

    /* Exit current screen */
    if (manager.stack_depth > 0) {
        screen_t *current = manager.screens[manager.stack[manager.stack_depth - 1]];
        if (current->on_exit) {
            current->on_exit(current->state);
        }
    }

    /* Push new screen */
    manager.stack[manager.stack_depth] = id;
    manager.stack_depth++;

    /* Enter new screen */
    screen_t *screen = manager.screens[id];
    if (screen->on_enter) {
        screen->on_enter(screen->state);
    }

    fprintf(stderr, "[screen] Pushed: %s (depth=%d)\n",
            screen->name, manager.stack_depth);
}

void screen_manager_pop(void)
{
    if (manager.stack_depth <= 1) {
        fprintf(stderr, "[screen] Can't pop last screen\n");
        return;
    }

    /* Exit current screen */
    screen_t *current = manager.screens[manager.stack[manager.stack_depth - 1]];
    if (current->on_exit) {
        current->on_exit(current->state);
    }

    manager.stack_depth--;

    /* Re-enter previous screen */
    screen_t *prev = manager.screens[manager.stack[manager.stack_depth - 1]];
    if (prev->on_enter) {
        prev->on_enter(prev->state);
    }

    fprintf(stderr, "[screen] Popped to: %s (depth=%d)\n",
            prev->name, manager.stack_depth);
}

void screen_manager_replace(screen_id_t id)
{
    if (manager.stack_depth <= 0) {
        screen_manager_push(id);
        return;
    }

    /* Exit current */
    screen_t *current = manager.screens[manager.stack[manager.stack_depth - 1]];
    if (current->on_exit) {
        current->on_exit(current->state);
    }

    /* Replace top of stack */
    manager.stack[manager.stack_depth - 1] = id;

    /* Enter new */
    screen_t *screen = manager.screens[id];
    if (screen->on_enter) {
        screen->on_enter(screen->state);
    }

    fprintf(stderr, "[screen] Replaced with: %s\n", screen->name);
}

screen_t *screen_manager_current(void)
{
    if (manager.stack_depth <= 0) return NULL;
    return manager.screens[manager.stack[manager.stack_depth - 1]];
}

/* ─── Render ─────────────────────────────────────────────────────────────── */

void screen_manager_render(void)
{
    screen_t *current = screen_manager_current();
    if (current && current->render) {
        current->render(current->state);
    }
}

/* ─── Input ──────────────────────────────────────────────────────────────── */

bool screen_manager_handle_key(const key_event_t *event)
{
    /* Global key handling (always available) */
    if (event->type == KEY_EVENT_PRESS && event->key == KEY_BACK) {
        if (manager.stack_depth > 1) {
            screen_manager_pop();
            return true;
        }
    }

    /* Delegate to current screen */
    screen_t *current = screen_manager_current();
    if (current && current->on_key) {
        return current->on_key(current->state, event);
    }

    return false;
}
