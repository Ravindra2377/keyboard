/* keypad_sim.c — drop-in keypad.c replacement for macOS testing */
#include <SDL2/SDL.h>
#include <string.h>
#include <time.h>
#include "stealth_ui.h"

/* ─── Timestamp Helper ───────────────────────────────────────────────────── */
static uint32_t get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int keypad_init(keypad_t *kp, const char *device) {
    (void)device;
    kp->fd = -1;
    kp->hold_threshold_ms = 500;
    memset(kp->key_press_time, 0, sizeof(kp->key_press_time));
    return 0;
}

void keypad_destroy(keypad_t *kp) {
    (void)kp;
}

static keypad_key_t sdl_to_stealth(SDL_Keycode sym) {
    switch (sym) {
        case SDLK_1: return KEY_1;
        case SDLK_2: return KEY_2;
        case SDLK_3: return KEY_3;
        case SDLK_4: return KEY_4;
        case SDLK_5: return KEY_5;
        case SDLK_6: return KEY_6;
        case SDLK_7: return KEY_7;
        case SDLK_8: return KEY_8;
        case SDLK_9: return KEY_9;
        case SDLK_0: return KEY_0;
        case SDLK_RETURN: return KEY_CALL;
        case SDLK_ESCAPE: return KEY_END;
        case SDLK_BACKSPACE: return KEY_STAR;
        case SDLK_HASH:  return KEY_HASH;
        case SDLK_UP:    return KEY_UP;
        case SDLK_DOWN:  return KEY_DOWN;
        case SDLK_LEFT:  return KEY_LEFT;
        case SDLK_RIGHT: return KEY_RIGHT;
        case SDLK_SPACE: return KEY_CENTER;
        default: return KEY_NONE;
    }
}

bool keypad_poll(keypad_t *kp, key_event_t *event, int timeout_ms) {
    (void)timeout_ms; /* In SDL, we just poll instantly */
    SDL_Event e;
    uint32_t now = get_time_ms();

    /* Check for pending hold events first */
    for (int i = 0; i <= KEY_BACK; i++) {
        if (kp->key_press_time[i] > 0) {
            uint32_t elapsed = now - kp->key_press_time[i];
            if (elapsed >= kp->hold_threshold_ms) {
                event->key = (keypad_key_t)i;
                event->type = KEY_EVENT_HOLD;
                event->timestamp_ms = now;
                kp->key_press_time[i] = 0;  /* Only fire hold once */
                return true;
            }
        }
    }

    if (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            exit(0);
        }
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            if (e.key.repeat) return false; /* Ignore OS auto-repeat */
            
            keypad_key_t key = sdl_to_stealth(e.key.keysym.sym);
            if (key == KEY_NONE) return false;
            
            event->key = key;
            event->timestamp_ms = now;
            
            if (e.type == SDL_KEYDOWN) {
                event->type = KEY_EVENT_PRESS;
                kp->key_press_time[key] = now;
            } else {
                event->type = KEY_EVENT_RELEASE;
                kp->key_press_time[key] = 0;
            }
            return true;
        }
    }
    
    SDL_Delay(10); /* Prevent 100% CPU in the loop since we aren't doing a blocking poll */
    return false;
}
