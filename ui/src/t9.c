/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — T9 Multi-Tap Text Input
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Classic multi-tap text input for T9 keypad phones.
 * Each number key maps to multiple characters; pressing the same key
 * cycles through them. After a timeout (500ms), the character is committed.
 *
 * Key mapping:
 *   1: .,-?!'@#$%    (punctuation)
 *   2: abc2           3: def3
 *   4: ghi4           5: jkl5
 *   6: mno6           7: pqrs7
 *   8: tuv8           9: wxyz9
 *   0: (space)0       *: +*           #: (mode switch)
 */

#include "stealth_ui.h"

#include <string.h>
#include <time.h>

/* ─── Multi-Tap Character Tables ─────────────────────────────────────────── */

static const char *multitap_lower[] = {
    [KEY_0] = " 0",
    [KEY_1] = ".,!?'-@#$%&1",
    [KEY_2] = "abc2",
    [KEY_3] = "def3",
    [KEY_4] = "ghi4",
    [KEY_5] = "jkl5",
    [KEY_6] = "mno6",
    [KEY_7] = "pqrs7",
    [KEY_8] = "tuv8",
    [KEY_9] = "wxyz9",
    [KEY_STAR] = "+*",
    [KEY_HASH] = NULL,  /* Mode switch, not a character */
};

static const char *multitap_upper[] = {
    [KEY_0] = " 0",
    [KEY_1] = ".,!?'-@#$%&1",
    [KEY_2] = "ABC2",
    [KEY_3] = "DEF3",
    [KEY_4] = "GHI4",
    [KEY_5] = "JKL5",
    [KEY_6] = "MNO6",
    [KEY_7] = "PQRS7",
    [KEY_8] = "TUV8",
    [KEY_9] = "WXYZ9",
    [KEY_STAR] = "+*",
    [KEY_HASH] = NULL,
};

static const char *multitap_numeric[] = {
    [KEY_0] = "0",
    [KEY_1] = "1",
    [KEY_2] = "2",
    [KEY_3] = "3",
    [KEY_4] = "4",
    [KEY_5] = "5",
    [KEY_6] = "6",
    [KEY_7] = "7",
    [KEY_8] = "8",
    [KEY_9] = "9",
    [KEY_STAR] = "+*#",
    [KEY_HASH] = NULL,
};

/* ─── Input Modes ────────────────────────────────────────────────────────── */

typedef enum {
    INPUT_MODE_LOWER = 0,   /* abc */
    INPUT_MODE_UPPER,       /* ABC */
    INPUT_MODE_NUMERIC,     /* 123 */
    INPUT_MODE_COUNT
} input_mode_t;

static const char *mode_names[] = {
    [INPUT_MODE_LOWER]   = "abc",
    [INPUT_MODE_UPPER]   = "ABC",
    [INPUT_MODE_NUMERIC] = "123",
};

/* ─── T9 State ───────────────────────────────────────────────────────────── */

/* Extend t9_input_t with mode tracking */
static input_mode_t current_mode = INPUT_MODE_LOWER;

#define MULTITAP_TIMEOUT_MS 600  /* ms before character is committed */

/* ─── Timestamp Helper ───────────────────────────────────────────────────── */

static uint32_t t9_get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* ─── T9 Functions ───────────────────────────────────────────────────────── */

void t9_init(t9_input_t *t9)
{
    memset(t9, 0, sizeof(*t9));
    t9->multi_tap_key = -1;
    t9->multi_tap_index = 0;
    t9->last_tap_time = 0;
    t9->predictive = false;
    current_mode = INPUT_MODE_LOWER;
}

static const char **get_current_table(void)
{
    switch (current_mode) {
        case INPUT_MODE_LOWER:   return multitap_lower;
        case INPUT_MODE_UPPER:   return multitap_upper;
        case INPUT_MODE_NUMERIC: return multitap_numeric;
        default:                 return multitap_lower;
    }
}

static void commit_pending(t9_input_t *t9)
{
    /* If there's a pending multi-tap character, commit it */
    if (t9->multi_tap_key >= 0 && t9->multi_tap_key <= KEY_STAR) {
        const char **table = get_current_table();
        const char *chars = table[t9->multi_tap_key];
        if (chars) {
            int len = strlen(chars);
            int idx = t9->multi_tap_index % len;
            if (t9->length < (int)sizeof(t9->buffer) - 1) {
                t9->buffer[t9->cursor] = chars[idx];
                t9->cursor++;
                t9->length++;
                t9->buffer[t9->length] = '\0';
            }
        }
    }
    t9->multi_tap_key = -1;
    t9->multi_tap_index = 0;
}

void t9_handle_key(t9_input_t *t9, keypad_key_t key)
{
    uint32_t now = t9_get_time_ms();

    /* Mode switch (# key) */
    if (key == KEY_HASH) {
        commit_pending(t9);
        current_mode = (current_mode + 1) % INPUT_MODE_COUNT;
        return;
    }

    /* Only handle number keys, star for text input */
    if (key > KEY_STAR) return;

    const char **table = get_current_table();
    const char *chars = table[key];
    if (!chars) return;

    /* Check if this is the same key pressed within timeout */
    if (t9->multi_tap_key == (int)key &&
        (now - t9->last_tap_time) < MULTITAP_TIMEOUT_MS) {
        /* Cycle to next character */
        t9->multi_tap_index++;

        /* Update the character in the buffer (replace, don't append) */
        int len = strlen(chars);
        int idx = t9->multi_tap_index % len;

        /* The character is already in the buffer at cursor-1 (from first tap
         * or previous cycle). Overwrite it. */
        if (t9->cursor > 0) {
            t9->buffer[t9->cursor - 1] = chars[idx];
        }
    } else {
        /* Different key or timeout expired — commit previous and start new */
        if (t9->multi_tap_key >= 0) {
            /* Previous character was already placed by first tap */
            /* Nothing to do — it's already in the buffer */
        }

        t9->multi_tap_key = (int)key;
        t9->multi_tap_index = 0;

        /* Place first character */
        if (t9->length < (int)sizeof(t9->buffer) - 1) {
            t9->buffer[t9->cursor] = chars[0];
            t9->cursor++;
            t9->length++;
            t9->buffer[t9->length] = '\0';
        }
    }

    t9->last_tap_time = now;
}

const char *t9_get_text(const t9_input_t *t9)
{
    return t9->buffer;
}

void t9_clear(t9_input_t *t9)
{
    memset(t9->buffer, 0, sizeof(t9->buffer));
    t9->cursor = 0;
    t9->length = 0;
    t9->multi_tap_key = -1;
    t9->multi_tap_index = 0;
}

/* ─── Helper: Get Current Mode Name ──────────────────────────────────────── */

const char *t9_mode_name(void)
{
    return mode_names[current_mode];
}

/* ─── Helper: Handle Backspace ───────────────────────────────────────────── */

void t9_backspace(t9_input_t *t9)
{
    /* Cancel any pending multi-tap */
    t9->multi_tap_key = -1;
    t9->multi_tap_index = 0;

    if (t9->cursor > 0) {
        t9->cursor--;
        t9->length--;
        /* Shift remaining characters left */
        memmove(&t9->buffer[t9->cursor],
                &t9->buffer[t9->cursor + 1],
                t9->length - t9->cursor);
        t9->buffer[t9->length] = '\0';
    }
}

/* ─── Helper: Commit Timeout ─────────────────────────────────────────────── */

void t9_tick(t9_input_t *t9)
{
    /* Call this periodically to auto-commit after timeout */
    if (t9->multi_tap_key >= 0) {
        uint32_t now = t9_get_time_ms();
        if ((now - t9->last_tap_time) >= MULTITAP_TIMEOUT_MS) {
            /* Timeout expired — character is already in buffer, just reset */
            t9->multi_tap_key = -1;
            t9->multi_tap_index = 0;
        }
    }
}
