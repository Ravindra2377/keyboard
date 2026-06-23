/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * StealthOS — Framebuffer Renderer
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Direct pixel rendering to Linux framebuffer (/dev/fb0).
 * No GPU, no X11, no Wayland — just mmap'd pixel memory.
 *
 * Supports:
 *   - ARGB8888 pixel format (most common)
 *   - Filled rectangles, rounded rectangles, circles
 *   - Vertical gradients
 *   - Line drawing (Bresenham)
 *   - Alpha blending
 */

#include "stealth_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

/* ─── Framebuffer Init/Destroy ───────────────────────────────────────────── */

int fb_init(framebuffer_t *fb, const char *device)
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (!device) device = "/dev/fb0";

    fb->fd = open(device, O_RDWR);
    if (fb->fd < 0) {
        perror("fb_init: open framebuffer");
        return -1;
    }

    /* Get variable screen info */
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("fb_init: FBIOGET_VSCREENINFO");
        close(fb->fd);
        return -1;
    }

    /* Get fixed screen info */
    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("fb_init: FBIOGET_FSCREENINFO");
        close(fb->fd);
        return -1;
    }

    fb->width  = vinfo.xres;
    fb->height = vinfo.yres;
    fb->bpp    = vinfo.bits_per_pixel;
    fb->stride = finfo.line_length;

    if (fb->bpp != 32) {
        fprintf(stderr, "fb_init: only 32bpp supported (got %d)\n", fb->bpp);
        close(fb->fd);
        return -1;
    }

    /* Map framebuffer memory */
    size_t fb_size = fb->stride * fb->height;
    fb->pixels = (uint32_t *)mmap(NULL, fb_size,
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fb->fd, 0);

    if (fb->pixels == MAP_FAILED) {
        perror("fb_init: mmap");
        close(fb->fd);
        return -1;
    }

    fprintf(stderr, "[fb] Initialized: %dx%d @ %dbpp (stride=%d)\n",
            fb->width, fb->height, fb->bpp, fb->stride);

    return 0;
}

void fb_destroy(framebuffer_t *fb)
{
    if (fb->pixels && fb->pixels != MAP_FAILED) {
        size_t fb_size = fb->stride * fb->height;
        munmap(fb->pixels, fb_size);
    }
    if (fb->fd >= 0) {
        close(fb->fd);
    }
    memset(fb, 0, sizeof(*fb));
    fb->fd = -1;
}

/* ─── Color Helpers ──────────────────────────────────────────────────────── */

static inline uint8_t color_r(color_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t color_g(color_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t color_b(color_t c) { return c & 0xFF; }
static inline uint8_t color_a(color_t c) { return (c >> 24) & 0xFF; }

static inline color_t blend_pixel(color_t dst, color_t src)
{
    uint8_t sa = color_a(src);
    if (sa == 0xFF) return src;
    if (sa == 0x00) return dst;

    uint8_t da = 255 - sa;
    uint8_t r = (color_r(src) * sa + color_r(dst) * da) / 255;
    uint8_t g = (color_g(src) * sa + color_g(dst) * da) / 255;
    uint8_t b = (color_b(src) * sa + color_b(dst) * da) / 255;

    return COLOR_RGB(r, g, b);
}

static inline color_t lerp_color(color_t a, color_t b, int t, int max)
{
    uint8_t r = color_r(a) + (color_r(b) - color_r(a)) * t / max;
    uint8_t g = color_g(a) + (color_g(b) - color_g(a)) * t / max;
    uint8_t bl = color_b(a) + (color_b(b) - color_b(a)) * t / max;
    return COLOR_RGB(r, g, bl);
}

/* ─── Drawing Primitives ─────────────────────────────────────────────────── */

void fb_clear(framebuffer_t *fb, color_t color)
{
    /* Fast path: fill entire buffer with a single color */
    uint32_t *p = fb->pixels;
    int total = (fb->stride / 4) * fb->height;
    for (int i = 0; i < total; i++) {
        p[i] = color;
    }
}

void fb_pixel(framebuffer_t *fb, int x, int y, color_t color)
{
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;

    uint32_t *row = (uint32_t *)((uint8_t *)fb->pixels + y * fb->stride);
    if (color_a(color) == 0xFF) {
        row[x] = color;
    } else {
        row[x] = blend_pixel(row[x], color);
    }
}

void fb_rect(framebuffer_t *fb, int x, int y, int w, int h, color_t color)
{
    /* Clip to screen */
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > fb->width) ? fb->width : x + w;
    int y1 = (y + h > fb->height) ? fb->height : y + h;

    for (int row = y0; row < y1; row++) {
        uint32_t *p = (uint32_t *)((uint8_t *)fb->pixels + row * fb->stride);
        if (color_a(color) == 0xFF) {
            /* Fast fill — no blending needed */
            for (int col = x0; col < x1; col++) {
                p[col] = color;
            }
        } else {
            for (int col = x0; col < x1; col++) {
                p[col] = blend_pixel(p[col], color);
            }
        }
    }
}

void fb_rect_rounded(framebuffer_t *fb, int x, int y, int w, int h,
                     int radius, color_t color)
{
    if (radius <= 0) {
        fb_rect(fb, x, y, w, h, color);
        return;
    }

    /* Clamp radius */
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;

    /* Fill center rectangle */
    fb_rect(fb, x + radius, y, w - 2 * radius, h, color);

    /* Fill left and right strips */
    fb_rect(fb, x, y + radius, radius, h - 2 * radius, color);
    fb_rect(fb, x + w - radius, y + radius, radius, h - 2 * radius, color);

    /* Draw four rounded corners */
    for (int dy = 0; dy < radius; dy++) {
        for (int dx = 0; dx < radius; dx++) {
            int dist_sq = (radius - dx - 1) * (radius - dx - 1) +
                          (radius - dy - 1) * (radius - dy - 1);
            if (dist_sq <= radius * radius) {
                /* Top-left */
                fb_pixel(fb, x + dx, y + dy, color);
                /* Top-right */
                fb_pixel(fb, x + w - 1 - dx, y + dy, color);
                /* Bottom-left */
                fb_pixel(fb, x + dx, y + h - 1 - dy, color);
                /* Bottom-right */
                fb_pixel(fb, x + w - 1 - dx, y + h - 1 - dy, color);
            }
        }
    }
}

void fb_line(framebuffer_t *fb, int x0, int y0, int x1, int y1, color_t color)
{
    /* Bresenham's line algorithm */
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        fb_pixel(fb, x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void fb_circle(framebuffer_t *fb, int cx, int cy, int r, color_t color)
{
    /* Midpoint circle algorithm */
    int x = r;
    int y = 0;
    int err = 1 - r;

    while (x >= y) {
        fb_pixel(fb, cx + x, cy + y, color);
        fb_pixel(fb, cx + y, cy + x, color);
        fb_pixel(fb, cx - y, cy + x, color);
        fb_pixel(fb, cx - x, cy + y, color);
        fb_pixel(fb, cx - x, cy - y, color);
        fb_pixel(fb, cx - y, cy - x, color);
        fb_pixel(fb, cx + y, cy - x, color);
        fb_pixel(fb, cx + x, cy - y, color);

        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void fb_circle_filled(framebuffer_t *fb, int cx, int cy, int r, color_t color)
{
    for (int y = -r; y <= r; y++) {
        int half_w = (int)(0.5 + sqrt((double)(r * r - y * y)));
        fb_rect(fb, cx - half_w, cy + y, half_w * 2 + 1, 1, color);
    }
}

void fb_gradient_v(framebuffer_t *fb, int x, int y, int w, int h,
                   color_t top, color_t bottom)
{
    for (int row = 0; row < h; row++) {
        color_t c = lerp_color(top, bottom, row, h);
        fb_rect(fb, x, y + row, w, 1, c);
    }
}

void fb_flip(framebuffer_t *fb)
{
    /* For single-buffer setup, no-op (writes go directly to display).
     * For double-buffered setup, swap page with FBIOPAN_DISPLAY. */
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo) == 0) {
        vinfo.activate = FB_ACTIVATE_NOW;
        ioctl(fb->fd, FBIOPAN_DISPLAY, &vinfo);
    }
}
