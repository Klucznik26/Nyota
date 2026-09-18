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
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include "font8x8.h"

static NyotaHost g_nyhost;

static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static int g_video;
static int g_gfx;
static int g_graph_closed;
static uint32_t g_gw = 640, g_gh = 480;
static uint8_t g_shift;
static int g_dirty;
static const char *g_input_feed;
static int g_input_pos;

#define HOST_MAX_SPRITES 64
#define HOST_MAX_SCREENS 16
#define HOST_MAX_UI_WINDOWS 32
static SDL_Texture *g_sprite_tex[HOST_MAX_SPRITES];
static SDL_Texture *g_screen_tex[HOST_MAX_SCREENS];
static uint32_t g_active_screen;

typedef struct {
    int used;
    int32_t handle;
    int32_t parent_handle;
    SDL_Window *win;
    SDL_Renderer *ren;
    uint32_t w, h;
    NyotaUiBackground background;
    int has_background;
} HostUiWindow;

static HostUiWindow g_ui_windows[HOST_MAX_UI_WINDOWS];

static void host_ui_render_background_index(int idx);
static void host_ui_destroy_index(int idx);

static void posix_emit(char c) {
    fputc(c, stdout);
    if (c == '\n') fflush(stdout);
}

static uint64_t host_ticks(void);

static int host_ui_index_by_window_id(uint32_t id) {
    int i;
    for (i = 0; i < HOST_MAX_UI_WINDOWS; i++) {
        if (g_ui_windows[i].used && g_ui_windows[i].win &&
            SDL_GetWindowID(g_ui_windows[i].win) == id) return i;
    }
    return -1;
}

static int host_ui_index_by_handle(int32_t handle) {
    int i;
    for (i = 0; i < HOST_MAX_UI_WINDOWS; i++)
        if (g_ui_windows[i].used && g_ui_windows[i].handle == handle) return i;
    return -1;
}

static void host_pump(void) {
    SDL_Event e;
    if (!g_video) return;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            g_graph_closed = 1;
            continue;
        }
        if (e.type == SDL_WINDOWEVENT) {
            int ui = host_ui_index_by_window_id(e.window.windowID);
            if (ui >= 0) {
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                    e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int w = 0, h = 0;
                    SDL_GetWindowSize(g_ui_windows[ui].win, &w, &h);
                    if (w > 0 && h > 0) {
                        g_ui_windows[ui].w = (uint32_t)w;
                        g_ui_windows[ui].h = (uint32_t)h;
                        if (g_ui_windows[ui].has_background)
                            host_ui_render_background_index(ui);
                    }
                } else if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    host_ui_destroy_index(ui);
                }
                continue;
            }
            if (g_win && SDL_GetWindowID(g_win) == e.window.windowID &&
                e.window.event == SDL_WINDOWEVENT_CLOSE) {
                g_graph_closed = 1;
                continue;
            }
        }
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_LSHIFT || k == SDLK_RSHIFT) g_shift = 1;
            if (k == SDLK_ESCAPE && g_win &&
                e.key.windowID == SDL_GetWindowID(g_win)) g_graph_closed = 1;
        }
        if (e.type == SDL_KEYUP) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_LSHIFT || k == SDLK_RSHIFT) g_shift = 0;
        }
    }
    if (g_dirty && g_ren && !g_graph_closed) {
        SDL_RenderPresent(g_ren);
        g_dirty = 0;
    }
}

static uint64_t host_ticks(void) {
    struct timespec ts;
    host_pump();
    if (g_video) SDL_Delay(1);
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

static int32_t host_file_read(const char *path, char *out, uint32_t cap, uint32_t *out_size) {
    FILE *f;
    long sz;
    size_t got;
    if (out_size) *out_size = 0;
    if (!path || !out || cap == 0) return -1;
    f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    sz = ftell(f);
    if (sz < 0 || (uint64_t)sz + 1ULL > cap) { fclose(f); return -2; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -1; }
    got = fread(out, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) return -1;
    if (memchr(out, '\0', got) != NULL) return -3;
    out[got] = '\0';
    if (out_size) *out_size = (uint32_t)got;
    return 0;
}

static int32_t host_file_write(const char *path, const char *data, uint32_t size, uint8_t append) {
    FILE *f;
    size_t wrote;
    if (!path || !data) return -1;
    f = fopen(path, append ? "ab" : "wb");
    if (!f) return -1;
    wrote = fwrite(data, 1, size, f);
    if (fclose(f) != 0) return -1;
    return wrote == size ? 0 : -1;
}

static int32_t host_file_delete(const char *path) {
    return path && unlink(path) == 0 ? 0 : -1;
}

static int32_t host_file_exists(const char *path) {
    struct stat st;
    if (!path || stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode) ? 1 : 0;
}

static int64_t host_file_size(const char *path) {
    struct stat st;
    if (!path || stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return -1;
    return (int64_t)st.st_size;
}

static int32_t host_copy_file(const char *src, const char *dst) {
    FILE *in, *out;
    unsigned char buf[16384];
    size_t n;
    int ok = 0;
    if (!src || !dst) return -1;
    in = fopen(src, "rb");
    if (!in) return -1;
    out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { ok = -1; break; }
    }
    if (ferror(in)) ok = -1;
    if (fclose(out) != 0) ok = -1;
    fclose(in);
    if (ok != 0) unlink(dst);
    return ok;
}

static int32_t host_file_copy(const char *src, const char *dst) {
    struct stat st;
    if (!src || stat(src, &st) != 0 || !S_ISREG(st.st_mode)) return -1;
    return host_copy_file(src, dst);
}

static int32_t host_file_move(const char *src, const char *dst) {
    return src && dst && rename(src, dst) == 0 ? 0 : -1;
}

static int32_t host_dir_exists(const char *path) {
    struct stat st;
    if (!path || stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

static int32_t host_dir_create(const char *path) {
    if (!path) return -1;
    if (mkdir(path, 0777) == 0) return 0;
    if (errno == EEXIST && host_dir_exists(path)) return 0;
    return -1;
}

static int32_t host_dir_delete(const char *path) {
    return path && rmdir(path) == 0 ? 0 : -1;
}

static int32_t host_dir_move(const char *src, const char *dst) {
    return src && dst && rename(src, dst) == 0 ? 0 : -1;
}

static int32_t host_dir_copy_recursive(const char *src, const char *dst) {
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    char sp[2048], dp[2048];
    if (!src || !dst || stat(src, &st) != 0 || !S_ISDIR(st.st_mode)) return -1;
    if (host_dir_create(dst) != 0) return -1;
    dir = opendir(src);
    if (!dir) return -1;
    while ((ent = readdir(dir)) != NULL) {
        size_t sl, dl, nl;
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        sl = strlen(src); dl = strlen(dst); nl = strlen(ent->d_name);
        if (sl + nl + 2 > sizeof(sp) || dl + nl + 2 > sizeof(dp)) {
            closedir(dir);
            return -1;
        }
        snprintf(sp, sizeof(sp), "%s/%s", src, ent->d_name);
        snprintf(dp, sizeof(dp), "%s/%s", dst, ent->d_name);
        if (lstat(sp, &st) != 0) { closedir(dir); return -1; }
        if (S_ISDIR(st.st_mode)) {
            if (host_dir_copy_recursive(sp, dp) != 0) { closedir(dir); return -1; }
        } else if (S_ISREG(st.st_mode)) {
            if (host_copy_file(sp, dp) != 0) { closedir(dir); return -1; }
        } else {
            /* Bezpieczny profil: nie podążamy za symlinkami ani plikami specjalnymi. */
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);
    return 0;
}

static int32_t host_dir_copy(const char *src, const char *dst) {
    return host_dir_copy_recursive(src, dst);
}

static int32_t host_dir_list(const char *path, char *out, uint32_t cap, uint32_t *out_size) {
    DIR *dir;
    struct dirent *ent;
    uint32_t used = 0;
    if (out_size) *out_size = 0;
    if (!path || !out || cap == 0) return -1;
    dir = opendir(path);
    if (!dir) return -1;
    while ((ent = readdir(dir)) != NULL) {
        uint32_t n;
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        if (strchr(ent->d_name, '\n')) { closedir(dir); return -1; }
        n = (uint32_t)strlen(ent->d_name);
        if ((uint64_t)used + n + 2ULL > cap) { closedir(dir); return -2; }
        memcpy(out + used, ent->d_name, n);
        used += n;
        out[used++] = '\n';
    }
    closedir(dir);
    if (used) used--;
    out[used] = '\0';
    if (out_size) *out_size = used;
    return 0;
}


static int video_ensure(void) {
    if (g_video) return 1;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Nyota UI/GRAPH: SDL_Init: %s\n", SDL_GetError());
        return 0;
    }
    (void)IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    g_video = 1;
    return 1;
}

static void host_ui_lerp_color(const NyotaUiBackground *bg, double t,
                               uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
    uint32_t n = bg->color_count;
    uint32_t i;
    double scaled, f;
    const NyotaColor *c0, *c1;
    if (n == 0) { *r = *g = *b = 0; *a = 255; return; }
    if (n == 1) {
        *r = bg->colors[0].r; *g = bg->colors[0].g;
        *b = bg->colors[0].b; *a = bg->colors[0].a; return;
    }
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    scaled = t * (double)(n - 1);
    i = (uint32_t)scaled;
    if (i >= n - 1) { i = n - 2; f = 1.0; }
    else f = scaled - (double)i;
    c0 = &bg->colors[i];
    c1 = &bg->colors[i + 1];
    *r = (uint8_t)((double)c0->r + ((double)c1->r - c0->r) * f + 0.5);
    *g = (uint8_t)((double)c0->g + ((double)c1->g - c0->g) * f + 0.5);
    *b = (uint8_t)((double)c0->b + ((double)c1->b - c0->b) * f + 0.5);
    *a = (uint8_t)((double)c0->a + ((double)c1->a - c0->a) * f + 0.5);
}

static double host_ui_shape_t(const NyotaUiBackground *bg,
                              double dx, double dy, double rx, double ry) {
    const double pi = 3.14159265358979323846;
    double rad = -(double)bg->angle_deg * pi / 180.0;
    double cs = cos(rad), sn = sin(rad);
    double lx = dx * cs - dy * sn;
    double ly = dx * sn + dy * cs;
    double ax = fabs(lx), ay = fabs(ly);
    if (rx < 1.0) rx = 1.0;
    if (ry < 1.0) ry = 1.0;

    switch (bg->shape) {
        case NYOTA_UI_SHAPE_CIRCLE: {
            double mr = sqrt(rx * rx + ry * ry);
            return mr > 0.0 ? sqrt(lx * lx + ly * ly) / mr : 0.0;
        }
        case NYOTA_UI_SHAPE_ELLIPSE:
            return sqrt((lx * lx) / (rx * rx) + (ly * ly) / (ry * ry)) / 1.41421356237;
        case NYOTA_UI_SHAPE_SQUARE: {
            double rr = rx > ry ? rx : ry;
            double m = ax > ay ? ax : ay;
            return rr > 0.0 ? m / rr : 0.0;
        }
        case NYOTA_UI_SHAPE_RECT: {
            double tx = ax / rx, ty = ay / ry;
            return tx > ty ? tx : ty;
        }
        case NYOTA_UI_SHAPE_DIAMOND:
            return 0.5 * (ax / rx + ay / ry);
        case NYOTA_UI_SHAPE_STAR: {
            double mr = sqrt(rx * rx + ry * ry);
            double theta = atan2(ly, lx);
            double modulation = 0.62 + 0.38 * (0.5 + 0.5 * cos(5.0 * theta));
            double base = mr > 0.0 ? sqrt(lx * lx + ly * ly) / mr : 0.0;
            return modulation > 0.01 ? base / modulation : base;
        }
        case NYOTA_UI_SHAPE_EGG: {
            double yn = ly / ry;
            double erx = rx * (1.0 + 0.30 * yn);
            if (erx < 1.0) erx = 1.0;
            return sqrt((lx * lx) / (erx * erx) + (ly * ly) / (ry * ry)) / 1.41421356237;
        }
        default:
            return 0.0;
    }
}

static double host_ui_gradient_t(const NyotaUiBackground *bg,
                                 uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    const double pi = 3.14159265358979323846;
    double wx = w > 1 ? (double)(w - 1) : 1.0;
    double hy = h > 1 ? (double)(h - 1) : 1.0;
    if (bg->kind == NYOTA_UI_BG_LINEAR) {
        if (bg->direction == NYOTA_UI_DIR_HORIZONTAL) return (double)x / wx;
        if (bg->direction == NYOTA_UI_DIR_DIAG_DOWN)
            return 0.5 * ((double)x / wx + (double)y / hy);
        if (bg->direction == NYOTA_UI_DIR_DIAG_UP)
            return 0.5 * ((double)x / wx + (1.0 - (double)y / hy));
        return (double)y / hy;
    }

    {
        double cx = bg->center_mode == NYOTA_UI_POS_CENTER ? wx / 2.0 : (double)bg->center_x;
        double cy = bg->center_mode == NYOTA_UI_POS_CENTER ? hy / 2.0 : (double)bg->center_y;
        double dx = (double)x - cx, dy = (double)y - cy;
        double rx1 = fabs(cx), rx2 = fabs(wx - cx);
        double ry1 = fabs(cy), ry2 = fabs(hy - cy);
        double rx = rx1 > rx2 ? rx1 : rx2;
        double ry = ry1 > ry2 ? ry1 : ry2;
        if (rx < 1.0) rx = 1.0;
        if (ry < 1.0) ry = 1.0;

        if (bg->kind == NYOTA_UI_BG_SHAPE)
            return host_ui_shape_t(bg, dx, dy, rx, ry);

        if (bg->kind == NYOTA_UI_BG_SPIRAL) {
            double maxr = sqrt(rx * rx + ry * ry);
            double radial = maxr > 0.0 ? sqrt(dx * dx + dy * dy) / maxr : 0.0;
            double theta = atan2(dy, dx) - (double)bg->angle_deg * pi / 180.0;
            double angular = theta / (2.0 * pi);
            double phase;
            if (bg->spiral_direction == NYOTA_UI_SPIRAL_CCW) angular = -angular;
            phase = radial * (double)(bg->turns ? bg->turns : 1) + angular;
            phase -= floor(phase);
            if (phase < 0.0) phase += 1.0;
            return phase;
        }
    }
    return 0.0;
}

static int host_ui_render_gradient(HostUiWindow *u) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    uint32_t x, y;
    if (!u || !u->ren || u->w == 0 || u->h == 0) return -1;
    surface = SDL_CreateRGBSurfaceWithFormat(0, (int)u->w, (int)u->h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return -1;
    if (SDL_LockSurface(surface) != 0) { SDL_FreeSurface(surface); return -1; }
    for (y = 0; y < u->h; y++) {
        uint32_t *row = (uint32_t *)((uint8_t *)surface->pixels + y * surface->pitch);
        for (x = 0; x < u->w; x++) {
            uint8_t r, g, b, a;
            double t = host_ui_gradient_t(&u->background, x, y, u->w, u->h);
            host_ui_lerp_color(&u->background, t, &r, &g, &b, &a);
            row[x] = SDL_MapRGBA(surface->format, r, g, b, a);
        }
    }
    SDL_UnlockSurface(surface);
    texture = SDL_CreateTextureFromSurface(u->ren, surface);
    SDL_FreeSurface(surface);
    if (!texture) return -1;
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(u->ren, 0, 0, 0, 255);
    SDL_RenderClear(u->ren);
    SDL_RenderCopy(u->ren, texture, NULL, NULL);
    SDL_RenderPresent(u->ren);
    SDL_DestroyTexture(texture);
    return 0;
}

static int host_ui_render_image(HostUiWindow *u) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    int sw, sh, dw, dh;
    SDL_Rect src, dst;
    if (!u || !u->ren || !u->background.image_path[0]) return -1;
    surface = IMG_Load(u->background.image_path);
    if (!surface) return -1;
    sw = surface->w; sh = surface->h;
    texture = SDL_CreateTextureFromSurface(u->ren, surface);
    SDL_FreeSurface(surface);
    if (!texture || sw <= 0 || sh <= 0) { if (texture) SDL_DestroyTexture(texture); return -1; }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(u->ren, 0, 0, 0, 255);
    SDL_RenderClear(u->ren);

    dst.x = 0; dst.y = 0; dst.w = (int)u->w; dst.h = (int)u->h;
    if (u->background.image_mode == NYOTA_UI_IMG_STRETCH) {
        SDL_RenderCopy(u->ren, texture, NULL, &dst);
    } else if (u->background.image_mode == NYOTA_UI_IMG_NATIVE) {
        dst.w = sw; dst.h = sh;
        SDL_RenderCopy(u->ren, texture, NULL, &dst);
    } else if (u->background.image_mode == NYOTA_UI_IMG_TILE) {
        int y, x;
        for (y = 0; y < (int)u->h; y += sh)
            for (x = 0; x < (int)u->w; x += sw) {
                dst.x = x; dst.y = y; dst.w = sw; dst.h = sh;
                SDL_RenderCopy(u->ren, texture, NULL, &dst);
            }
    } else if (u->background.image_mode == NYOTA_UI_IMG_FIT) {
        if ((int64_t)sw * (int64_t)u->h > (int64_t)sh * (int64_t)u->w) {
            dw = (int)u->w;
            dh = (int)((int64_t)sh * u->w / sw);
        } else {
            dh = (int)u->h;
            dw = (int)((int64_t)sw * u->h / sh);
        }
        dst.w = dw; dst.h = dh;
        dst.x = ((int)u->w - dw) / 2; dst.y = ((int)u->h - dh) / 2;
        SDL_RenderCopy(u->ren, texture, NULL, &dst);
    } else {
        src.x = 0; src.y = 0; src.w = sw; src.h = sh;
        if ((int64_t)sw * (int64_t)u->h > (int64_t)sh * (int64_t)u->w) {
            src.w = (int)((int64_t)sh * u->w / u->h);
            src.x = (sw - src.w) / 2;
        } else {
            src.h = (int)((int64_t)sw * u->h / u->w);
            src.y = (sh - src.h) / 2;
        }
        SDL_RenderCopy(u->ren, texture, &src, &dst);
    }
    SDL_RenderPresent(u->ren);
    SDL_DestroyTexture(texture);
    return 0;
}

static void host_ui_render_background_index(int idx) {
    HostUiWindow *u;
    NyotaColor c;
    if (idx < 0 || idx >= HOST_MAX_UI_WINDOWS || !g_ui_windows[idx].used) return;
    u = &g_ui_windows[idx];
    if (!u->ren || !u->has_background) return;
    if (u->background.kind == NYOTA_UI_BG_IMAGE) {
        (void)host_ui_render_image(u);
        return;
    }
    if (u->background.kind == NYOTA_UI_BG_LINEAR ||
        u->background.kind == NYOTA_UI_BG_SHAPE ||
        u->background.kind == NYOTA_UI_BG_SPIRAL) {
        (void)host_ui_render_gradient(u);
        return;
    }
    c = u->background.colors[0];
    if (c.mode == NYOTA_COLOR_BACKDROP) return;
    SDL_SetRenderDrawColor(u->ren, c.r, c.g, c.b, c.a);
    SDL_RenderClear(u->ren);
    SDL_RenderPresent(u->ren);
}

static int32_t host_ui_win_create(const char *name, int32_t parent_handle,
                                  uint32_t w, uint32_t h, int32_t x, int32_t y,
                                  uint8_t position_mode, uint8_t resizable) {
    int slot = -1, i;
    int px = 0, py = 0, pw = 0, ph = 0;
    int sx = SDL_WINDOWPOS_UNDEFINED, sy = SDL_WINDOWPOS_UNDEFINED;
    Uint32 flags = 0;
    SDL_Window *win;
    SDL_Renderer *ren;
    if (!name || !name[0] || !w || !h || !video_ensure()) return -1;
    for (i = 0; i < HOST_MAX_UI_WINDOWS; i++) if (!g_ui_windows[i].used) { slot = i; break; }
    if (slot < 0) return -1;

    if (parent_handle > 0) {
        int pi = host_ui_index_by_handle(parent_handle);
        if (pi < 0) return -1;
        SDL_GetWindowPosition(g_ui_windows[pi].win, &px, &py);
        SDL_GetWindowSize(g_ui_windows[pi].win, &pw, &ph);
    }

    if (position_mode == NYOTA_UI_POS_CENTER) {
        if (parent_handle > 0) {
            sx = px + (pw - (int)w) / 2;
            sy = py + (ph - (int)h) / 2;
        } else {
            sx = SDL_WINDOWPOS_CENTERED; sy = SDL_WINDOWPOS_CENTERED;
        }
    } else if (position_mode == NYOTA_UI_POS_XY) {
        sx = parent_handle > 0 ? px + x : x;
        sy = parent_handle > 0 ? py + y : y;
    }
    if (resizable) flags |= SDL_WINDOW_RESIZABLE;
    win = SDL_CreateWindow(name, sx, sy, (int)w, (int)h, flags);
    if (!win) return -1;
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren) { SDL_DestroyWindow(win); return -1; }

    g_ui_windows[slot].used = 1;
    g_ui_windows[slot].handle = slot + 1;
    g_ui_windows[slot].parent_handle = parent_handle;
    g_ui_windows[slot].win = win;
    g_ui_windows[slot].ren = ren;
    g_ui_windows[slot].w = w;
    g_ui_windows[slot].h = h;
    g_ui_windows[slot].has_background = 0;
    return g_ui_windows[slot].handle;
}

static int32_t host_ui_win_set_title(int32_t handle, const char *title) {
    int i = host_ui_index_by_handle(handle);
    if (i < 0 || !title) return -1;
    SDL_SetWindowTitle(g_ui_windows[i].win, title);
    return 0;
}

static int32_t host_ui_win_set_icon(int32_t handle, const char *path) {
    int i = host_ui_index_by_handle(handle);
    SDL_Surface *surface;
    if (i < 0 || !path || !path[0]) return -1;
    surface = IMG_Load(path);
    if (!surface) return -1;
    SDL_SetWindowIcon(g_ui_windows[i].win, surface);
    SDL_FreeSurface(surface);
    return 0;
}

static int32_t host_ui_win_set_background(int32_t handle, const NyotaUiBackground *background) {
    int i = host_ui_index_by_handle(handle);
    if (i < 0 || !background) return -1;
    if (background->kind == NYOTA_UI_BG_COLOR &&
        background->colors[0].mode == NYOTA_COLOR_BACKDROP) return -2;
    g_ui_windows[i].background = *background;
    g_ui_windows[i].has_background = 1;
    if (background->kind == NYOTA_UI_BG_IMAGE)
        return host_ui_render_image(&g_ui_windows[i]);
    if (background->kind == NYOTA_UI_BG_LINEAR ||
        background->kind == NYOTA_UI_BG_SHAPE ||
        background->kind == NYOTA_UI_BG_SPIRAL)
        return host_ui_render_gradient(&g_ui_windows[i]);
    host_ui_render_background_index(i);
    return 0;
}

static void host_ui_destroy_index(int idx) {
    int i;
    int32_t handle;
    if (idx < 0 || idx >= HOST_MAX_UI_WINDOWS || !g_ui_windows[idx].used) return;
    handle = g_ui_windows[idx].handle;
    for (i = 0; i < HOST_MAX_UI_WINDOWS; i++)
        if (g_ui_windows[i].used && g_ui_windows[i].parent_handle == handle)
            host_ui_destroy_index(i);
    if (g_ui_windows[idx].ren) SDL_DestroyRenderer(g_ui_windows[idx].ren);
    if (g_ui_windows[idx].win) SDL_DestroyWindow(g_ui_windows[idx].win);
    memset(&g_ui_windows[idx], 0, sizeof(g_ui_windows[idx]));
}

static void host_ui_win_destroy(int32_t handle) {
    int i = host_ui_index_by_handle(handle);
    if (i >= 0) host_ui_destroy_index(i);
}

static void gfx_ensure(void) {
    if (g_gfx) return;
    if (!video_ensure()) return;
    g_win = SDL_CreateWindow("Nyota", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             (int)g_gw, (int)g_gh, 0);
    if (!g_win) return;
    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED);
    if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, 0);
    if (!g_ren) return;
    g_gfx = 1;
    g_graph_closed = 0;
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_RenderPresent(g_ren);
}

static void host_setres(uint32_t w, uint32_t h) {
    int i;
    g_gw = w ? w : 640;
    g_gh = h ? h : 480;
    gfx_ensure();
    if (g_gfx && g_win) {
        /* Zmiana GRAPH unieważnia wszystkie cele off-screen poprzedniego trybu. */
        SDL_SetRenderTarget(g_ren, NULL);
        g_active_screen = 0;
        for (i = 1; i < HOST_MAX_SCREENS; i++) {
            if (g_screen_tex[i]) {
                SDL_DestroyTexture(g_screen_tex[i]);
                g_screen_tex[i] = 0;
            }
        }
        SDL_SetWindowSize(g_win, (int)g_gw, (int)g_gh);
        SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
        SDL_RenderClear(g_ren);
        SDL_RenderPresent(g_ren);
    }
}

static int32_t host_screen_open(uint32_t id, uint32_t w, uint32_t h, uint32_t color_mode) {
    SDL_Texture *tex;
    SDL_Texture *saved;
    (void)color_mode;
    gfx_ensure();
    if (!g_gfx || !g_ren || id == 0 || id >= HOST_MAX_SCREENS || g_screen_tex[id])
        return -1;
    tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_RGBA8888,
                            SDL_TEXTUREACCESS_TARGET, (int)w, (int)h);
    if (!tex) return -1;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    saved = SDL_GetRenderTarget(g_ren);
    if (SDL_SetRenderTarget(g_ren, tex) != 0) {
        SDL_DestroyTexture(tex);
        return -1;
    }
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_SetRenderTarget(g_ren, saved);
    g_screen_tex[id] = tex;
    return 0;
}

static int32_t host_screen_set(uint32_t id) {
    if (!g_gfx || !g_ren) return -1;
    if (id == 0) {
        if (SDL_SetRenderTarget(g_ren, NULL) != 0) return -1;
        g_active_screen = 0;
        return 0;
    }
    if (id >= HOST_MAX_SCREENS || !g_screen_tex[id]) return -1;
    if (SDL_SetRenderTarget(g_ren, g_screen_tex[id]) != 0) return -1;
    g_active_screen = id;
    return 0;
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


static int32_t host_sprite_load(const char *path) {
    SDL_Surface *surface;
    SDL_Texture *tex;
    int i;
    if (!path || !path[0]) return -1;
    gfx_ensure();
    if (!g_gfx || !g_ren) return -1;
    surface = SDL_LoadBMP(path);
    if (!surface) return -1;
    tex = SDL_CreateTextureFromSurface(g_ren, surface);
    SDL_FreeSurface(surface);
    if (!tex) return -1;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    for (i = 0; i < HOST_MAX_SPRITES; i++) {
        if (!g_sprite_tex[i]) {
            g_sprite_tex[i] = tex;
            return (int32_t)(i + 1);
        }
    }
    SDL_DestroyTexture(tex);
    return -1;
}

static void host_sprite_free(int32_t handle) {
    int idx = (int)handle - 1;
    if (idx < 0 || idx >= HOST_MAX_SPRITES) return;
    if (g_sprite_tex[idx]) {
        SDL_DestroyTexture(g_sprite_tex[idx]);
        g_sprite_tex[idx] = 0;
    }
}

static void host_sprite_draw(int32_t handle, int32_t x, int32_t y,
                             uint32_t w, uint32_t h) {
    SDL_Rect dst;
    int idx = (int)handle - 1;
    if (!g_gfx || !g_ren || idx < 0 || idx >= HOST_MAX_SPRITES ||
        !g_sprite_tex[idx] || w == 0 || h == 0) return;
    dst.x = (int)x;
    dst.y = (int)y;
    dst.w = (int)w;
    dst.h = (int)h;
    SDL_RenderCopy(g_ren, g_sprite_tex[idx], 0, &dst);
    g_dirty = 1;
}

static void host_sprite_draw_frame(int32_t handle, uint32_t frame,
                                   uint32_t frame_count,
                                   int32_t x, int32_t y,
                                   uint32_t w, uint32_t h) {
    SDL_Rect src, dst;
    int idx = (int)handle - 1;
    int tw = 0, th = 0, fw;
    if (!g_gfx || !g_ren || idx < 0 || idx >= HOST_MAX_SPRITES ||
        !g_sprite_tex[idx] || frame_count == 0 || frame >= frame_count ||
        w == 0 || h == 0) return;
    if (SDL_QueryTexture(g_sprite_tex[idx], 0, 0, &tw, &th) != 0) return;
    if (tw <= 0 || th <= 0 || (uint32_t)tw < frame_count) return;
    fw = tw / (int)frame_count;
    if (fw <= 0) return;
    src.x = (int)frame * fw;
    src.y = 0;
    src.w = fw;
    src.h = th;
    dst.x = (int)x;
    dst.y = (int)y;
    dst.w = (int)w;
    dst.h = (int)h;
    SDL_RenderCopy(g_ren, g_sprite_tex[idx], &src, &dst);
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
    int i;
    for (i = 0; i < HOST_MAX_UI_WINDOWS; i++)
        if (g_ui_windows[i].used) host_ui_destroy_index(i);
    if (g_gfx) {
        for (i = 0; i < HOST_MAX_SPRITES; i++) {
            if (g_sprite_tex[i]) {
                SDL_DestroyTexture(g_sprite_tex[i]);
                g_sprite_tex[i] = 0;
            }
        }
        for (i = 1; i < HOST_MAX_SCREENS; i++) {
            if (g_screen_tex[i]) {
                SDL_DestroyTexture(g_screen_tex[i]);
                g_screen_tex[i] = 0;
            }
        }
        if (g_ren) SDL_DestroyRenderer(g_ren);
        if (g_win) SDL_DestroyWindow(g_win);
        g_ren = 0; g_win = 0; g_gfx = 0;
    }
    if (g_video) {
        IMG_Quit();
        SDL_Quit();
        g_video = 0;
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

static int host_has_visible_windows(void) {
    int i;
    if (g_gfx && !g_graph_closed) return 1;
    for (i = 0; i < HOST_MAX_UI_WINDOWS; i++) if (g_ui_windows[i].used) return 1;
    return 0;
}

static void wait_close(void) {
    if (!g_video || getenv("NYOTA_NO_WAIT")) return;
    fprintf(stderr, "[Nyota] zamknij okna programu, aby zakonczyc.\n");
    while (host_has_visible_windows()) {
        host_pump();
        SDL_Delay(10);
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
    g_nyhost.file_read = host_file_read;
    g_nyhost.file_write = host_file_write;
    g_nyhost.file_delete = host_file_delete;
    g_nyhost.file_copy = host_file_copy;
    g_nyhost.file_move = host_file_move;
    g_nyhost.file_exists = host_file_exists;
    g_nyhost.file_size = host_file_size;
    g_nyhost.dir_create = host_dir_create;
    g_nyhost.dir_delete = host_dir_delete;
    g_nyhost.dir_copy = host_dir_copy;
    g_nyhost.dir_move = host_dir_move;
    g_nyhost.dir_exists = host_dir_exists;
    g_nyhost.dir_list = host_dir_list;
    g_nyhost.gfx_clear = host_clear;
    g_nyhost.gfx_rect = host_rect;
    g_nyhost.gfx_text = host_text;
    g_nyhost.gfx_mode = host_setres;
    g_nyhost.gfx_screen_open = host_screen_open;
    g_nyhost.gfx_screen_set = host_screen_set;
    g_nyhost.gfx_sprite_load = host_sprite_load;
    g_nyhost.gfx_sprite_free = host_sprite_free;
    g_nyhost.gfx_sprite_draw = host_sprite_draw;
    g_nyhost.gfx_sprite_draw_frame = host_sprite_draw_frame;
    g_nyhost.ui_win_create = host_ui_win_create;
    g_nyhost.ui_win_set_title = host_ui_win_set_title;
    g_nyhost.ui_win_set_icon = host_ui_win_set_icon;
    g_nyhost.ui_win_set_background = host_ui_win_set_background;
    g_nyhost.ui_win_destroy = host_ui_win_destroy;
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
