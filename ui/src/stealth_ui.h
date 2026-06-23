/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — stealth-ui
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Framebuffer-based phone interface for keypad devices.
 * Renders directly to /dev/fb0 with zero dependencies beyond libc.
 *
 * Architecture:
 *   - Framebuffer renderer (fb.c)      → pixel-level drawing to /dev/fb0
 *   - Font engine (font.c)             → stb_truetype TTF rendering
 *   - Keypad handler (keypad.c)        → /dev/input/eventX processing
 *   - Screen manager (screen.c)        → screen stack and transitions
 *   - Individual screens (screens/...) -> dial, sms, contacts, settings
 *   - Main loop (main.c)              → event loop, screen dispatch
 *
 * Build:
 *   make                    # native (for testing on Linux desktop)
 *   make CROSS_COMPILE=arm-linux-gnueabihf-  # cross-compile for target
 */

#ifndef STEALTH_UI_H
#define STEALTH_UI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ─── Version ────────────────────────────────────────────────────────────── */

#define STEALTH_VERSION_MAJOR 0
#define STEALTH_VERSION_MINOR 1
#define STEALTH_VERSION_PATCH 0

/* ─── Display Constants ──────────────────────────────────────────────────── */

/* Default for a typical keypad phone LCD (240x320 QVGA) */
#define DEFAULT_SCREEN_WIDTH   240
#define DEFAULT_SCREEN_HEIGHT  320
#define DEFAULT_BPP            32    /* bits per pixel (ARGB8888) */

/* ─── Color System ───────────────────────────────────────────────────────── */

/* ARGB8888 color format */
typedef uint32_t color_t;

#define COLOR_ARGB(a, r, g, b) \
    (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | \
     ((uint32_t)(g) << 8)  | ((uint32_t)(b)))

#define COLOR_RGB(r, g, b) COLOR_ARGB(0xFF, r, g, b)

/* StealthOS color palette — dark, minimal, secure-feeling */
#define COL_BG_PRIMARY     COLOR_RGB(0x0A, 0x0A, 0x0F)  /* Near-black */
#define COL_BG_SECONDARY   COLOR_RGB(0x14, 0x14, 0x1E)  /* Dark grey */
#define COL_BG_CARD        COLOR_RGB(0x1C, 0x1C, 0x2A)  /* Card bg */
#define COL_BG_SELECTED    COLOR_RGB(0x1E, 0x2D, 0x3D)  /* Selected item */
#define COL_FG_PRIMARY     COLOR_RGB(0xE8, 0xE8, 0xF0)  /* White text */
#define COL_FG_SECONDARY   COLOR_RGB(0x8A, 0x8A, 0x9A)  /* Grey text */
#define COL_FG_DIM         COLOR_RGB(0x50, 0x50, 0x60)  /* Dimmed text */
#define COL_ACCENT         COLOR_RGB(0x00, 0xD4, 0x7E)  /* Green accent */
#define COL_ACCENT_DIM     COLOR_RGB(0x00, 0x8A, 0x52)  /* Dim green */
#define COL_DANGER         COLOR_RGB(0xFF, 0x3B, 0x30)  /* Red / end call */
#define COL_WARNING        COLOR_RGB(0xFF, 0xCC, 0x02)  /* Yellow warning */
#define COL_CALL_GREEN     COLOR_RGB(0x34, 0xC7, 0x59)  /* Call button */
#define COL_CALL_RED       COLOR_RGB(0xFF, 0x3B, 0x30)  /* End call */
#define COL_ENCRYPTED      COLOR_RGB(0x00, 0xD4, 0x7E)  /* Encrypted indicator */
#define COL_SIGNAL_BAR     COLOR_RGB(0x00, 0xD4, 0x7E)  /* Signal strength */

/* ─── Key Mapping ────────────────────────────────────────────────────────── */

/* Standard T9 keypad layout */
typedef enum {
    KEY_0 = 0,
    KEY_1, KEY_2, KEY_3,
    KEY_4, KEY_5, KEY_6,
    KEY_7, KEY_8, KEY_9,
    KEY_STAR,           /* * key */
    KEY_HASH,           /* # key */
    KEY_CALL,           /* Green call button */
    KEY_END,            /* Red end/power button */
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_CENTER,         /* D-pad center / OK */
    KEY_SOFT_LEFT,      /* Left soft key */
    KEY_SOFT_RIGHT,     /* Right soft key */
    KEY_BACK,
    KEY_NONE = 0xFF
} keypad_key_t;

/* Key event types */
typedef enum {
    KEY_EVENT_PRESS = 0,
    KEY_EVENT_RELEASE = 1,
    KEY_EVENT_HOLD = 2      /* Long press (>500ms) */
} key_event_type_t;

typedef struct {
    keypad_key_t key;
    key_event_type_t type;
    uint32_t timestamp_ms;
} key_event_t;

/* ─── Screen System ──────────────────────────────────────────────────────── */

typedef enum {
    SCREEN_IDLE = 0,        /* Lock/idle screen with clock */
    SCREEN_PIN_ENTRY,       /* PIN unlock screen */
    SCREEN_HOME,            /* Main menu (after unlock) */
    SCREEN_DIALER,          /* Phone dialer */
    SCREEN_INCALL,          /* Active call screen */
    SCREEN_SMS_LIST,        /* SMS inbox */
    SCREEN_SMS_COMPOSE,     /* Compose SMS (T9 input) */
    SCREEN_SMS_VIEW,        /* View single SMS */
    SCREEN_CONTACTS,        /* Contact list */
    SCREEN_CONTACT_VIEW,    /* View single contact */
    SCREEN_SETTINGS,        /* Settings menu */
    SCREEN_ENCRYPTION_STATUS, /* Encryption status screen */
    SCREEN_COUNT
} screen_id_t;

/* Screen interface — every screen implements these callbacks */
typedef struct {
    screen_id_t id;
    const char *name;

    /* Lifecycle */
    void (*on_enter)(void *state);
    void (*on_exit)(void *state);

    /* Rendering */
    void (*render)(void *state);

    /* Input handling — return true if the event was consumed */
    bool (*on_key)(void *state, const key_event_t *event);

    /* Per-screen state (allocated by screen, freed on exit) */
    void *state;
} screen_t;

/* ─── Framebuffer Interface ──────────────────────────────────────────────── */

typedef struct {
    uint32_t *pixels;       /* Mapped pixel buffer */
    int width;
    int height;
    int stride;             /* Bytes per scanline */
    int bpp;                /* Bits per pixel */
    int fd;                 /* Framebuffer fd */
} framebuffer_t;

/* Initialize/destroy framebuffer */
int fb_init(framebuffer_t *fb, const char *device);
void fb_destroy(framebuffer_t *fb);

/* Drawing primitives */
void fb_clear(framebuffer_t *fb, color_t color);
void fb_pixel(framebuffer_t *fb, int x, int y, color_t color);
void fb_rect(framebuffer_t *fb, int x, int y, int w, int h, color_t color);
void fb_rect_rounded(framebuffer_t *fb, int x, int y, int w, int h,
                     int radius, color_t color);
void fb_line(framebuffer_t *fb, int x0, int y0, int x1, int y1, color_t color);
void fb_circle(framebuffer_t *fb, int cx, int cy, int r, color_t color);
void fb_circle_filled(framebuffer_t *fb, int cx, int cy, int r, color_t color);
void fb_gradient_v(framebuffer_t *fb, int x, int y, int w, int h,
                   color_t top, color_t bottom);

/* Text rendering (uses stb_truetype internally) */
void font_init(const char *path);
void fb_text(framebuffer_t *fb, int x, int y, const char *text,
             int font_size, color_t color);
int fb_text_width(const char *text, int font_size);
void fb_text_centered(framebuffer_t *fb, int y, const char *text,
                      int font_size, color_t color);

/* Flush to display (for double-buffered setups) */
void fb_flip(framebuffer_t *fb);

/* ─── Keypad Interface ───────────────────────────────────────────────────── */

typedef struct {
    int fd;                 /* Input device fd */
    uint32_t hold_threshold_ms;  /* ms before KEY_EVENT_HOLD fires */
    uint32_t key_press_time[KEY_BACK + 1]; /* Track press timestamps */
} keypad_t;

int keypad_init(keypad_t *kp, const char *device);
void keypad_destroy(keypad_t *kp);
bool keypad_poll(keypad_t *kp, key_event_t *event, int timeout_ms);

/* ─── UI Constants ─────────────────────────────────────────────────────────── */

#define MAX_PHONE_NUMBER 24

/* ─── Screen Manager ─────────────────────────────────────────────────────── */

void screen_manager_init(framebuffer_t *fb);
void screen_manager_push(screen_id_t id);
void screen_manager_pop(void);
void screen_manager_replace(screen_id_t id);
screen_t *screen_manager_current(void);
void screen_manager_render(void);
bool screen_manager_handle_key(const key_event_t *event);

/* ─── Status Bar ─────────────────────────────────────────────────────────── */

typedef struct {
    int signal_strength;    /* 0-5 bars */
    bool sim_present;
    bool encrypted;         /* True if comms are encrypted */
    int battery_percent;
    bool charging;
    char time_str[6];       /* "HH:MM" */
    char operator_name[32]; /* "StealthOS" or carrier name */
} status_info_t;

void statusbar_render(framebuffer_t *fb, const status_info_t *info);

/* ─── T9 Input ───────────────────────────────────────────────────────────── */

typedef struct {
    char buffer[256];
    int cursor;
    int length;
    bool predictive;        /* T9 predictive mode */
    int multi_tap_key;      /* Current multi-tap key (-1 if none) */
    int multi_tap_index;    /* Current character index within key */
    uint32_t last_tap_time; /* For multi-tap timeout */
} t9_input_t;

void t9_init(t9_input_t *t9);
void t9_handle_key(t9_input_t *t9, keypad_key_t key);
const char *t9_get_text(const t9_input_t *t9);
void t9_clear(t9_input_t *t9);

/* ─── Telephony Bridge (talks to oFono via D-Bus) ────────────────────────── */

typedef void (*call_state_cb)(const char *number, const char *state);
typedef void (*sms_received_cb)(const char *from, const char *text,
                                 uint32_t timestamp);

int telephony_init(void);
void telephony_destroy(void);
int telephony_dial(const char *number);
int telephony_hangup(void);
int telephony_answer(void);
int telephony_send_sms(const char *number, const char *text);
void telephony_set_call_callback(call_state_cb cb);
void telephony_set_sms_callback(sms_received_cb cb);

/* ─── Encrypted Storage ──────────────────────────────────────────────────── */

/* Storage is now managed via storage.h (SQLCipher) */

#endif /* STEALTH_UI_H */
