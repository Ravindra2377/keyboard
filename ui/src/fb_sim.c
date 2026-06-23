/* fb_sim.c — drop-in fb.c replacement for macOS testing */
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include "stealth_ui.h"

/* Match your actual keypad phone resolution */
#define SIM_WIDTH  240
#define SIM_HEIGHT 320

static SDL_Window   *g_window   = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture  *g_texture  = NULL;
static uint32_t     *g_pixels   = NULL;

/* Called by main.c instead of fb_open() */
int fb_open(const char *path) {
    (void)path;
    SDL_Init(SDL_INIT_VIDEO);
    g_window = SDL_CreateWindow("StealthOS Simulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SIM_WIDTH * 2, SIM_HEIGHT * 2, 0);  /* 2× scale */
    g_renderer = SDL_CreateRenderer(g_window, -1,
        SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(g_renderer, SIM_WIDTH, SIM_HEIGHT);
    g_texture = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SIM_WIDTH, SIM_HEIGHT);
    g_pixels = calloc(SIM_WIDTH * SIM_HEIGHT, sizeof(uint32_t));
    return 0;
}

/* Your fb.c calls this to get the pixel buffer */
uint32_t *fb_pixels(void) { return g_pixels; }
int fb_width(void)        { return SIM_WIDTH; }
int fb_height(void)       { return SIM_HEIGHT; }

/* Call this once per frame in your main loop */
void fb_flush(void) {
    SDL_UpdateTexture(g_texture, NULL, g_pixels,
                      SIM_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

void fb_close(void) {
    free(g_pixels);
    SDL_DestroyTexture(g_texture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}
