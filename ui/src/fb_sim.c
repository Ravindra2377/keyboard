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

/* Called by main.c instead of linux hardware init */
int fb_init(framebuffer_t *fb, const char *device) {
    (void)device;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
    
    g_window = SDL_CreateWindow("StealthOS Simulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SIM_WIDTH * 2, SIM_HEIGHT * 2, 0);  /* 2× scale */
    if (!g_window) return -1;
    
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_renderer) return -1;
    
    SDL_RenderSetLogicalSize(g_renderer, SIM_WIDTH, SIM_HEIGHT);
    g_texture = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SIM_WIDTH, SIM_HEIGHT);
    
    fb->width = SIM_WIDTH;
    fb->height = SIM_HEIGHT;
    fb->bpp = 32;
    fb->stride = SIM_WIDTH * sizeof(uint32_t);
    fb->pixels = calloc(SIM_WIDTH * SIM_HEIGHT, sizeof(uint32_t));
    fb->fd = -1;
    
    return 0;
}

/* Call this once per frame in your main loop */
void fb_flip(framebuffer_t *fb) {
    SDL_UpdateTexture(g_texture, NULL, fb->pixels, fb->stride);
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

void fb_destroy(framebuffer_t *fb) {
    free(fb->pixels);
    SDL_DestroyTexture(g_texture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}
