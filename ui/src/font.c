/*
 * StealthOS — Font Engine
 * Renders TrueType fonts directly to the framebuffer using stb_truetype.
 */

#include "stealth_ui.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static stbtt_fontinfo g_font;
static unsigned char  g_font_buf[1 << 20];  // 1MB font buffer
static bool g_font_initialized = false;

void font_init(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[font] Failed to open font %s\n", path);
        return;
    }
    size_t read_bytes = fread(g_font_buf, 1, sizeof(g_font_buf), f);
    fclose(f);

    if (read_bytes == 0) {
        fprintf(stderr, "[font] Failed to read font %s\n", path);
        return;
    }

    if (!stbtt_InitFont(&g_font, g_font_buf, stbtt_GetFontOffsetForIndex(g_font_buf, 0))) {
        fprintf(stderr, "[font] stbtt_InitFont failed\n");
        return;
    }

    g_font_initialized = true;
    fprintf(stderr, "[font] Initialized %s\n", path);
}

// Draw a single character to the framebuffer at (x, y)
static void fb_draw_char(framebuffer_t *fb, char ch, int x, int y,
                         float size_px, color_t color) {
    if (!g_font_initialized) return;

    float scale = stbtt_ScaleForPixelHeight(&g_font, size_px);
    int w, h, xoff, yoff;
    unsigned char *bitmap = stbtt_GetCodepointBitmap(
        &g_font, 0, scale, ch, &w, &h, &xoff, &yoff);

    if (!bitmap) return;

    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            uint8_t alpha = bitmap[row * w + col];
            if (alpha > 32) {  // threshold — skip near-transparent
                int px = x + xoff + col;
                int py = y + yoff + row;
                
                // Blend with alpha (simple alpha blending)
                color_t src_color = COLOR_ARGB(alpha, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
                fb_pixel(fb, px, py, src_color);
            }
        }
    }
    stbtt_FreeBitmap(bitmap, NULL);
}

void fb_text(framebuffer_t *fb, int x, int y, const char *text,
             int font_size, color_t color) {
    if (!text || !g_font_initialized) {
        // Fallback to placeholder if font not loaded
        if (!text || !text[0]) return;
        int len = strlen(text);
        int char_w = font_size * 5 / 8;
        for (int i = 0; i < len; i++) {
            fb_rect(fb, x + i * char_w, y, char_w - 1, font_size, color);
        }
        return;
    }

    float size_px = (float)font_size;
    float scale = stbtt_ScaleForPixelHeight(&g_font, size_px);
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&g_font, &ascent, &descent, &linegap);

    int cx = x;
    for (const char *p = text; *p; p++) {
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&g_font, *p, &advance, &lsb);
        fb_draw_char(fb, *p, cx, y + (int)(ascent * scale),
                     size_px, color);
        cx += (int)(advance * scale);
        // Kerning between this char and next
        if (*(p+1)) {
            cx += (int)(stbtt_GetCodepointKernAdvance(
                &g_font, *p, *(p+1)) * scale);
        }
    }
}

int fb_text_width(const char *text, int font_size) {
    if (!text) return 0;
    if (!g_font_initialized) {
        return strlen(text) * (font_size * 5 / 8);
    }
    
    float size_px = (float)font_size;
    float scale = stbtt_ScaleForPixelHeight(&g_font, size_px);
    
    int cx = 0;
    for (const char *p = text; *p; p++) {
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&g_font, *p, &advance, &lsb);
        cx += (int)(advance * scale);
        if (*(p+1)) {
            cx += (int)(stbtt_GetCodepointKernAdvance(&g_font, *p, *(p+1)) * scale);
        }
    }
    return cx;
}

void fb_text_centered(framebuffer_t *fb, int y, const char *text,
                      int font_size, color_t color) {
    int w = fb_text_width(text, font_size);
    fb_text(fb, (fb->width - w) / 2, y, text, font_size, color);
}
