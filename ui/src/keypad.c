/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — Keypad Input Handler
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Reads raw keypad events from /dev/input/eventX and translates them
 * into StealthOS key events with press/release/hold detection.
 *
 * Supports:
 *   - Standard T9 keypad (0-9, *, #)
 *   - Navigation keys (up/down/left/right/center)
 *   - Call/end buttons
 *   - Soft keys
 *   - Long press detection (configurable threshold)
 *
 * The keypad is typically a GPIO matrix scanned by the kernel's
 * matrix-keypad driver, which exposes events via /dev/input/eventX.
 */

#include "stealth_ui.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <linux/input.h>

/* ─── Linux Key Code → StealthOS Key Mapping ─────────────────────────────── */

/*
 * Linux input subsystem key codes (from linux/input-event-codes.h)
 * mapped to our keypad_key_t enum.
 *
 * The exact mapping depends on your device tree / keypad driver config.
 * This mapping works for standard GPIO matrix keypads configured with
 * the standard Linux key codes.
 */
static keypad_key_t linux_to_stealth(int linux_code)
{
    switch (linux_code) {
        /* Number keys */
        case KEY_KP0:
        case KEY_NUMERIC_0:     return KEY_0;
        case KEY_KP1:
        case KEY_NUMERIC_1:     return KEY_1;
        case KEY_KP2:
        case KEY_NUMERIC_2:     return KEY_2;
        case KEY_KP3:
        case KEY_NUMERIC_3:     return KEY_3;
        case KEY_KP4:
        case KEY_NUMERIC_4:     return KEY_4;
        case KEY_KP5:
        case KEY_NUMERIC_5:     return KEY_5;
        case KEY_KP6:
        case KEY_NUMERIC_6:     return KEY_6;
        case KEY_KP7:
        case KEY_NUMERIC_7:     return KEY_7;
        case KEY_KP8:
        case KEY_NUMERIC_8:     return KEY_8;
        case KEY_KP9:
        case KEY_NUMERIC_9:     return KEY_9;
        case KEY_NUMERIC_STAR:  return KEY_STAR;
        case KEY_NUMERIC_POUND: return KEY_HASH;

        /* Navigation */
        case KEY_UP:            return KEY_UP;
        case KEY_DOWN:          return KEY_DOWN;
        case KEY_LEFT:          return KEY_LEFT;
        case KEY_RIGHT:         return KEY_RIGHT;
        case KEY_ENTER:
        case KEY_KPENTER:
        case KEY_SELECT:        return KEY_CENTER;

        /* Phone keys */
        case KEY_SEND:
        case KEY_PHONE:         return KEY_CALL;
        case KEY_END:
        case KEY_POWER:         return KEY_END;
        case KEY_BACK:
        case KEY_ESC:           return KEY_BACK;

        /* Soft keys (typically F1/F2 on keypad phones) */
        case KEY_F1:
        case KEY_MENU:          return KEY_SOFT_LEFT;
        case KEY_F2:
        case KEY_CONTEXT_MENU:  return KEY_SOFT_RIGHT;

        default:                return KEY_NONE;
    }
}

/* ─── Timestamp Helper ───────────────────────────────────────────────────── */

static uint32_t get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* ─── Keypad Init/Destroy ────────────────────────────────────────────────── */

int keypad_init(keypad_t *kp, const char *device)
{
    if (!device) device = "/dev/input/event0";

    kp->fd = open(device, O_RDONLY | O_NONBLOCK);
    if (kp->fd < 0) {
        fprintf(stderr, "[keypad] Failed to open %s: %s\n",
                device, strerror(errno));
        return -1;
    }

    kp->hold_threshold_ms = 500;  /* 500ms for long press */
    memset(kp->key_press_time, 0, sizeof(kp->key_press_time));

    /* Print device name for debugging */
    char name[256] = "Unknown";
    ioctl(kp->fd, EVIOCGNAME(sizeof(name)), name);
    fprintf(stderr, "[keypad] Opened: %s (%s)\n", device, name);

    return 0;
}

void keypad_destroy(keypad_t *kp)
{
    if (kp->fd >= 0) {
        close(kp->fd);
    }
    kp->fd = -1;
}

/* ─── Keypad Polling ─────────────────────────────────────────────────────── */

bool keypad_poll(keypad_t *kp, key_event_t *event, int timeout_ms)
{
    struct pollfd pfd = {
        .fd = kp->fd,
        .events = POLLIN
    };

    /* Check for pending hold events first */
    uint32_t now = get_time_ms();
    for (int i = 0; i <= KEY_BACK; i++) {
        if (kp->key_press_time[i] > 0) {
            uint32_t elapsed = now - kp->key_press_time[i];
            if (elapsed >= kp->hold_threshold_ms) {
                /* Fire hold event */
                event->key = (keypad_key_t)i;
                event->type = KEY_EVENT_HOLD;
                event->timestamp_ms = now;
                kp->key_press_time[i] = 0;  /* Only fire hold once */
                return true;
            }
        }
    }

    /* Wait for input event */
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret <= 0) return false;

    /* Read raw Linux input event */
    struct input_event ev;
    ssize_t n = read(kp->fd, &ev, sizeof(ev));
    if (n != sizeof(ev)) return false;

    /* We only care about key events (EV_KEY) */
    if (ev.type != EV_KEY) return false;

    /* Map Linux key code to StealthOS key */
    keypad_key_t key = linux_to_stealth(ev.code);
    if (key == KEY_NONE) return false;

    event->key = key;
    event->timestamp_ms = now;

    if (ev.value == 1) {
        /* Key pressed */
        event->type = KEY_EVENT_PRESS;
        kp->key_press_time[key] = now;
    } else if (ev.value == 0) {
        /* Key released */
        event->type = KEY_EVENT_RELEASE;
        kp->key_press_time[key] = 0;
    } else if (ev.value == 2) {
        /* Key repeat (auto-repeat from kernel) — ignore, we handle hold ourselves */
        return false;
    }

    return true;
}
