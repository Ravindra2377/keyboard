/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — Main Entry Point
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Initializes framebuffer, keypad, telephony, and screen manager.
 * Runs the main event loop: poll keypad → handle input → render → repeat.
 *
 * The entire phone UI runs in this single-threaded loop at ~30fps.
 * No window manager, no compositor — just raw framebuffer rendering.
 */

#include "stealth_ui.h"
#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

/* ─── Globals ────────────────────────────────────────────────────────────── */

static volatile int running = 1;
static framebuffer_t fb;
static keypad_t kp;

/* ─── Signal Handler ─────────────────────────────────────────────────────── */

static void signal_handler(int sig)
{
    (void)sig;
    running = 0;
}

/* ─── Telephony is now in telephony.c ────────────────────────────────────── */

/* ─── Font rendering is now in font.c ────────────────────────────────────── */

/* ─── Storage is now in storage.c ────────────────────────────────────────── */

/* ─── Main ───────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    const char *fb_device = "/dev/fb0";
    const char *kp_device = "/dev/input/event0";

    /* Parse command line */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fb") == 0 && i + 1 < argc) {
            fb_device = argv[++i];
        } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            kp_device = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("StealthOS UI v%d.%d.%d\n",
                   STEALTH_VERSION_MAJOR, STEALTH_VERSION_MINOR,
                   STEALTH_VERSION_PATCH);
            printf("Usage: %s [--fb <device>] [--input <device>]\n", argv[0]);
            printf("  --fb <device>     Framebuffer device (default: /dev/fb0)\n");
            printf("  --input <device>  Keypad input device (default: /dev/input/event0)\n");
            return 0;
        }
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    fprintf(stderr, "\n");
    fprintf(stderr, "  ╔═══════════════════════════════════════╗\n");
    fprintf(stderr, "  ║  StealthOS v%d.%d.%d                    ║\n",
            STEALTH_VERSION_MAJOR, STEALTH_VERSION_MINOR,
            STEALTH_VERSION_PATCH);
    fprintf(stderr, "  ║  Encrypted Keypad Phone OS            ║\n");
    fprintf(stderr, "  ╚═══════════════════════════════════════╝\n");
    fprintf(stderr, "\n");

    /* ─── Initialize Subsystems ──────────────────────────────────────── */

    fprintf(stderr, "[main] Initializing framebuffer: %s\n", fb_device);
    if (fb_init(&fb, fb_device) < 0) {
        fprintf(stderr, "[main] FATAL: Cannot open framebuffer\n");
        fprintf(stderr, "[main] Make sure /dev/fb0 exists and you have permissions\n");
        return 1;
    }

    fprintf(stderr, "[main] Initializing keypad: %s\n", kp_device);
    if (keypad_init(&kp, kp_device) < 0) {
        fprintf(stderr, "[main] WARNING: Cannot open keypad, keyboard input disabled\n");
        /* Continue without keypad for display testing */
    }

    fprintf(stderr, "[main] Initializing telephony\n");
    telephony_init();

    fprintf(stderr, "[main] Initializing font engine\n");
    font_init("terminus.ttf");

    fprintf(stderr, "[main] Initializing encrypted storage\n");
    uint8_t key32[32] = {0}; /* TODO: Get from Argon2id initramfs */
    if (storage_open(key32) != 0) {
        fprintf(stderr, "[main] Storage error: db corruption or wrong PIN\n");
        sleep(3);
        /* reboot(RB_POWER_OFF); */
    }

    fprintf(stderr, "[main] Initializing screen manager\n");
    screen_manager_init(&fb);

    /* Start on idle screen */
    screen_manager_push(SCREEN_IDLE);

    fprintf(stderr, "[main] Entering main loop\n");

    /* ─── Main Event Loop ────────────────────────────────────────────── */

    /*
     * The main loop runs at approximately 30fps:
     * 1. Poll keypad for input events (33ms timeout = ~30fps)
     * 2. Handle any key events
     * 3. Render current screen
     * 4. Repeat
     *
     * This simple polling model works perfectly for a keypad phone
     * where there's no complex multi-touch or gesture handling.
     */

    struct timespec frame_start, frame_end;
    const int target_fps = 30;
    const long frame_time_ns = 1000000000L / target_fps;

    while (running) {
        clock_gettime(CLOCK_MONOTONIC, &frame_start);

        /* Poll for keypad input */
        key_event_t event;
        if (keypad_poll(&kp, &event, 0)) {
            /* Handle the key event */
            screen_manager_handle_key(&event);
        }

        /* Render current screen */
        screen_manager_render();

        /* Frame rate limiting */
        clock_gettime(CLOCK_MONOTONIC, &frame_end);
        long elapsed_ns = (frame_end.tv_sec - frame_start.tv_sec) * 1000000000L +
                          (frame_end.tv_nsec - frame_start.tv_nsec);
        long sleep_ns = frame_time_ns - elapsed_ns;
        if (sleep_ns > 0) {
            struct timespec sleep_time = {
                .tv_sec = 0,
                .tv_nsec = sleep_ns
            };
            nanosleep(&sleep_time, NULL);
        }
    }

    /* ─── Cleanup ────────────────────────────────────────────────────── */

    fprintf(stderr, "\n[main] Shutting down...\n");

    fb_clear(&fb, 0x00000000);
    fb_flip(&fb);

    telephony_destroy();
    keypad_destroy(&kp);
    fb_destroy(&fb);

    fprintf(stderr, "[main] Goodbye.\n");
    return 0;
}
