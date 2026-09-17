/* Host POSIX: PRINT (stdout w trybie tekstowym), GRAPH/INPUT/DELAY (SDL2).
 * Nyota jest językiem. Ten plik tylko podłącza backend Linuksa.
 *
 * Po GRAPH n: okno o rozdzielczości trybu, BOX/LINE/CIRCLE/CLEAR w oknie.
 * PRINT po GRAPH jest błędem języka (tak jak na AyoOS).
 * Bez GRAPH: stdout, bez okna — make test zostaje bez SDL.
 */

#include "../../src/nyota_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include "font8x8.h"

static NyotaHost g_nyhost;

static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static int g_gfx;
static uint32_t g_gw = 640, g_gh = 480;
static uint8_t g_shift;
static int g_dirty;
static const char *g_input_feed;
static int g_input_pos;

static void posix_emit(char c) {
    fputc(c, stdout);
    if (c == '\n') fflush(stdout);
}

static uint64_t host_ticks(void);

static void host_pump(void) {
    SDL_Event e;
    if (!g_gfx) return;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) exit(0);
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_LSHIFT || k == SDLK_RSHIFT) g_shift = 1;
            if (k == SDLK_ESCAPE) exit(0);
        }
        if (e.type == SDL_KEYUP) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_LSHIFT || k == SDLK_RSHIFT) g_shift = 0;
        }
    }
    if (g_dirty && g_ren) {
        SDL_RenderPresent(g_ren);
        g_dirty = 0;
    }
}

static uint64_t host_ticks(void) {
    struct timespec ts;
    host_pump();
    if (g_gfx) SDL_Delay(1);
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 100ULL + (uint64_t)(ts.tv_nsec / 10000000ULL);
}

static uint64_t host_unix(void) {
    return (uint64_t)time(NULL);
}

static uint32_t host_local_time_seconds(void) {
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    return (uint32_t)(tmv.tm_hour * 3600 + tmv.tm_min * 60 + tmv.tm_sec);
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
    if (!g_ren) return;
    g_gfx = 1;
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_RenderPresent(g_ren);
}

static void host_setres(uint32_t w, uint32_t h) {
    g_gw = w ? w : 640;
    g_gh = h ? h : 480;
    gfx_ensure();
    if (g_gfx && g_win) {
        SDL_SetWindowSize(g_win, (int)g_gw, (int)g_gh);
        SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
        SDL_RenderClear(g_ren);
        SDL_RenderPresent(g_ren);
    }
}

static void host_clear(uint8_t r, uint8_t g, uint8_t b) {
    /* Bez GRAPH nie otwieramy okna — CLEAR w trybie tekstowym to no-op. */
    if (!g_gfx) return;
    SDL_SetRenderDrawColor(g_ren, r, g, b, 255);
    SDL_RenderClear(g_ren);
    g_dirty = 1;
    host_pump();
}

static void host_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      uint8_t r, uint8_t g, uint8_t b) {
    SDL_Rect rc;
    if (!g_gfx) return;
    rc.x = (int)x; rc.y = (int)y; rc.w = (int)w; rc.h = (int)h;
    SDL_SetRenderDrawColor(g_ren, r, g, b, 255);
    SDL_RenderFillRect(g_ren, &rc);
    g_dirty = 1;
}

static void host_text(uint32_t x, uint32_t y, const char *text,
                      uint8_t r, uint8_t g, uint8_t b, uint32_t scale) {
    int px, i, row, col;
    unsigned char bits;
    if (!text) return;
    /* PRINT idzie przez emit (stdout). Tutaj tylko tekst na powierzchni GRAPH. */
    if (!g_gfx) return;
    if (scale < 1) scale = 1;
    SDL_SetRenderDrawColor(g_ren, r, g, b, 255);
    px = (int)x;
    for (i = 0; text[i]; i++) {
        unsigned c = (unsigned char)text[i];
        const unsigned char *glyph;
        if (c < 32 || c > 126) c = '?';
        glyph = FONT8[c - 32];
        for (row = 0; row < 8; row++) {
            bits = glyph[row];
            for (col = 0; col < 8; col++) {
                if (bits & (1u << col)) {
                    SDL_Rect rc;
                    rc.x = px + col * (int)scale;
                    rc.y = (int)y + row * (int)scale;
                    rc.w = (int)scale;
                    rc.h = (int)scale;
                    SDL_RenderFillRect(g_ren, &rc);
                }
            }
        }
        px += 8 * (int)scale;
    }
    g_dirty = 1;
}

static uint8_t key_to_scancode(SDL_Keycode k) {
    if (k == SDLK_RETURN) return 0x1C;
    if (k == SDLK_BACKSPACE) return 0x0E;
    if (k == SDLK_ESCAPE) return 0x01;
    if (k == SDLK_SPACE) return 0x39;
    if (k == SDLK_PERIOD) return 0x34;
    if (k == SDLK_MINUS) return 0x0C;
    if (k >= SDLK_1 && k <= SDLK_9) return (uint8_t)(0x02 + (k - SDLK_1));
    if (k == SDLK_0) return 0x0B;
    {
        const char *q = "qwertyuiop";
        const char *a = "asdfghjkl";
        const char *z = "zxcvbnm";
        int i;
        for (i = 0; q[i]; i++)
            if (k == q[i] || k == (q[i] - 32)) return (uint8_t)(0x10 + i);
        for (i = 0; a[i]; i++)
            if (k == a[i] || k == (a[i] - 32)) return (uint8_t)(0x1E + i);
        for (i = 0; z[i]; i++)
            if (k == z[i] || k == (z[i] - 32)) return (uint8_t)(0x2C + i);
    }
    return 0;
}

static uint8_t ascii_to_scancode(int c) {
    if (c == '\n' || c == '\r') return 0x1C;
    if (c == 8 || c == 127) return 0x0E;
    if (c == 27) return 0x01;
    if (c == ' ') return 0x39;
    if (c == '.') return 0x34;
    if (c == '-') return 0x0C;
    if (c >= 'A' && c <= 'Z') { g_shift = 1; c += 32; }
    if (c >= '1' && c <= '9') return (uint8_t)(0x02 + (c - '1'));
    if (c == '0') return 0x0B;
    {
        const char *q = "qwertyuiop";
        const char *a = "asdfghjkl";
        const char *z = "zxcvbnm";
        int i;
        for (i = 0; q[i]; i++) if (c == q[i]) return (uint8_t)(0x10 + i);
        for (i = 0; a[i]; i++) if (c == a[i]) return (uint8_t)(0x1E + i);
        for (i = 0; z[i]; i++) if (c == z[i]) return (uint8_t)(0x2C + i);
    }
    return 0;
}

static uint8_t host_waitkey(void) {
    if (g_input_feed) {
        unsigned char c;
        if (g_input_feed[g_input_pos]) {
            c = (unsigned char)g_input_feed[g_input_pos++];
            g_shift = 0;
            if (c >= 'A' && c <= 'Z') g_shift = 1;
            return ascii_to_scancode((int)c);
        }
        return 0x1C;
    }
    if (!g_gfx) {
        int c = getchar();
        g_shift = 0;
        if (c == EOF) return 0x01;
        return ascii_to_scancode(c);
    }
    {
        SDL_Event e;
        host_pump();
        for (;;) {
            if (!SDL_WaitEvent(&e)) return 0x01;
            if (e.type == SDL_QUIT) exit(0);
            if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_LSHIFT || e.key.keysym.sym == SDLK_RSHIFT)
                    g_shift = 0;
            }
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_LSHIFT || k == SDLK_RSHIFT) { g_shift = 1; continue; }
                if (k == SDLK_ESCAPE) exit(0);
                return key_to_scancode(k);
            }
        }
    }
}

static uint8_t host_mods(void) {
    return g_shift ? 1 : 0;
}

static uint8_t host_pointer_state(int32_t *x, int32_t *y) {
    int mx = 0, my = 0;
    uint32_t state;
    if (!g_gfx) {
        if (x) *x = 0;
        if (y) *y = 0;
        return 0;
    }
    host_pump();
    state = SDL_GetMouseState(&mx, &my);
    if (x) *x = (int32_t)mx;
    if (y) *y = (int32_t)my;
    return (state & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1U : 0U;
}

static void host_exit(void) {
    if (g_gfx) {
        SDL_DestroyRenderer(g_ren);
        SDL_DestroyWindow(g_win);
        SDL_Quit();
        g_gfx = 0;
    }
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

static void wait_close(void) {
    SDL_Event e;
    if (!g_gfx) return;
    if (getenv("NYOTA_NO_WAIT")) return;
    host_pump();
    fprintf(stderr, "[Nyota] GRAPH: ESC albo zamkniecie okna konczy.\n");
    while (SDL_WaitEvent(&e)) {
        if (e.type == SDL_QUIT) break;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) break;
    }
}

int main(int argc, char **argv) {
    char *src;
    const char *path;

    g_nyhost.emit = posix_emit;
    g_nyhost.unix_time = host_unix;
    g_nyhost.local_time_seconds = host_local_time_seconds;
    g_nyhost.ticks_100hz = host_ticks;
    g_nyhost.wait_key = host_waitkey;
    g_nyhost.key_mods = host_mods;
    g_nyhost.pointer_state = host_pointer_state;
    g_nyhost.gfx_clear = host_clear;
    g_nyhost.gfx_rect = host_rect;
    g_nyhost.gfx_text = host_text;
    g_nyhost.gfx_mode = host_setres;
    NyotaSetHost(&g_nyhost);
    g_input_feed = getenv("NYOTA_INPUT");
    g_input_pos = 0;

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
    wait_close();
    host_exit();
    return 0;
}
