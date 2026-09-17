/* Host POSIX: PRINT (stdout), INPUT (stdin), DELAY, GRAPH (SDL2).
 * Nyota jest językiem. Ten plik tylko podłącza backend Linuksa. */

#include "ayo_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>

AyoAPI *g_api;
static AyoAPI g_api_impl;

static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static int g_gfx;
static uint32_t g_gw = 640, g_gh = 480;
static int g_exit_req;

static void posix_emit(char c) {
    fputc(c, stdout);
    if (c == '\n') fflush(stdout);
}

static uint64_t host_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 100ULL + (uint64_t)(ts.tv_nsec / 10000000ULL);
}

static uint64_t host_unix(void) {
    return (uint64_t)time(NULL);
}

static void gfx_ensure(void) {
    if (g_gfx) return;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Nyota GRAPH: SDL_Init: %s\n", SDL_GetError());
        return;
    }
    g_win = SDL_CreateWindow("Nyota", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             (int)g_gw, (int)g_gh, 0);
    if (!g_win) return;
    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED);
    if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, 0);
    g_gfx = 1;
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_RenderPresent(g_ren);
}

static void host_setres(uint32_t w, uint32_t h) {
    g_gw = w ? w : 640;
    g_gh = h ? h : 480;
    if (g_gfx && g_win) SDL_SetWindowSize(g_win, (int)g_gw, (int)g_gh);
}

static void host_clear(uint8_t r, uint8_t g, uint8_t b) {
    gfx_ensure();
    if (!g_gfx) return;
    SDL_SetRenderDrawColor(g_ren, r, g, b, 255);
    SDL_RenderClear(g_ren);
    SDL_RenderPresent(g_ren);
}

static void host_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      uint8_t r, uint8_t g, uint8_t b) {
    SDL_Rect rc;
    gfx_ensure();
    if (!g_gfx) return;
    rc.x = (int)x; rc.y = (int)y; rc.w = (int)w; rc.h = (int)h;
    SDL_SetRenderDrawColor(g_ren, r, g, b, 255);
    SDL_RenderFillRect(g_ren, &rc);
    SDL_RenderPresent(g_ren);
}

static void host_text(uint32_t x, uint32_t y, const char *text,
                      uint8_t r, uint8_t g, uint8_t b, uint32_t scale) {
    (void)x; (void)y; (void)r; (void)g; (void)b; (void)scale;
    if (text) fputs(text, stdout);
}

static uint8_t host_waitkey(void) {
    if (!g_gfx) {
        int c = getchar();
        if (c == EOF) return 27;
        if (c == '\n') return 13;
        return (uint8_t)c;
    }
    SDL_Event e;
    for (;;) {
        while (SDL_WaitEvent(&e)) {
            if (e.type == SDL_QUIT) { g_exit_req = 1; return 27; }
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_RETURN) return 13;
                if (k == SDLK_ESCAPE) return 27;
                if (k == SDLK_BACKSPACE) return 8;
                if (k >= 32 && k < 127) return (uint8_t)k;
            }
        }
    }
}

static uint8_t host_mods(void) { return 0; }

static int host_readfile(const char *path, uint8_t *buffer, uint32_t buffer_size,
                         uint32_t *out_size) {
    FILE *f;
    size_t n;
    if (!path || !buffer) return -1;
    f = fopen(path, "rb");
    if (!f) return -1;
    n = fread(buffer, 1, buffer_size, f);
    fclose(f);
    if (out_size) *out_size = (uint32_t)n;
    return 0;
}

static void host_exit(void) {
    g_exit_req = 1;
}

#define NYOTA_EMBEDDED 1
#include "../../src/nyota.c"

static char *load_file(const char *path) {
    FILE *f = fopen(path, "rb");
    long sz;
    char *buf;
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return 0; }
    buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return 0; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return 0; }
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    char *src;
    const char *path;

    g_api_impl.SetResolution = host_setres;
    g_api_impl.ClearScreen = host_clear;
    g_api_impl.DrawRect = host_rect;
    g_api_impl.DrawText = host_text;
    g_api_impl.WaitForKey = host_waitkey;
    g_api_impl.GetKeyModifiers = host_mods;
    g_api_impl.ReadFile = host_readfile;
    g_api_impl.GetTicks = host_ticks;
    g_api_impl.Exit = host_exit;
    g_api_impl.GetUnixTime = host_unix;
    g_api = &g_api_impl;

    if (argc < 2) {
        fprintf(stderr, "uzycie: nyota <plik.nyo>\n");
        return 2;
    }
    path = argv[1];
    src = load_file(path);
    if (!src) {
        fprintf(stderr, "nie moge odczytac: %s\n", path);
        return 1;
    }
    NyotaEmbedRun(src, posix_emit);
    fputc('\n', stdout);
    free(src);
    if (g_gfx) {
        SDL_DestroyRenderer(g_ren);
        SDL_DestroyWindow(g_win);
        SDL_Quit();
    }
    return 0;
}
