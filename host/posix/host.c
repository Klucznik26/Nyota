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
#define HOST_MAX_UI_CONTROLS 128
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

typedef struct {
    int used;
    int32_t handle;
    int32_t window_handle;
    int32_t parent_control_handle;
    NyotaUiControlSpec spec;
    uint8_t prev_down;
    uint8_t hover;
    uint8_t dropped;
    uint8_t combo_open;
    int32_t combo_hover;
    char drop_items[NYOTA_UI_DROP_MAX];
} HostUiControl;

static HostUiControl g_host_ui_controls[HOST_MAX_UI_CONTROLS];

static void host_ui_render_background_index(int idx);
static void host_ui_destroy_index(int idx);
static void host_ui_redraw_controls(int window_index);

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

static int host_ui_control_index_by_handle(int32_t handle) {
    int i;
    for (i = 0; i < HOST_MAX_UI_CONTROLS; i++)
        if (g_host_ui_controls[i].used && g_host_ui_controls[i].handle == handle) return i;
    return -1;
}

static int host_ui_control_rect_index(int idx, SDL_Rect *out) {
    HostUiControl *c;
    SDL_Rect parent;
    int wi;
    if (!out || idx < 0 || idx >= HOST_MAX_UI_CONTROLS || !g_host_ui_controls[idx].used) return 0;
    c = &g_host_ui_controls[idx];
    wi = host_ui_index_by_handle(c->window_handle);
    if (wi < 0) return 0;
    if (c->parent_control_handle > 0) {
        int pi = host_ui_control_index_by_handle(c->parent_control_handle);
        if (pi < 0 || !host_ui_control_rect_index(pi, &parent)) return 0;
    } else {
        parent.x = 0; parent.y = 0;
        parent.w = (int)g_ui_windows[wi].w; parent.h = (int)g_ui_windows[wi].h;
    }
    if (c->spec.kind == NYOTA_UI_CTRL_TAB && c->parent_control_handle > 0) {
        int pi = host_ui_control_index_by_handle(c->parent_control_handle);
        int hh = 30;
        if (pi >= 0 && g_host_ui_controls[pi].spec.kind == NYOTA_UI_CTRL_TABS) {
            int fs = (int)c->spec.tab_font_size;
            if (fs + 12 > hh) hh = fs + 12;
            out->x = parent.x; out->y = parent.y + hh;
            out->w = parent.w; out->h = parent.h > hh ? parent.h - hh : 1;
            return 1;
        }
    }
    out->w = (int)c->spec.w; out->h = (int)c->spec.h;
    if (c->spec.position_mode == NYOTA_UI_POS_CENTER) {
        out->x = parent.x + (parent.w - out->w) / 2;
        out->y = parent.y + (parent.h - out->h) / 2;
    } else {
        out->x = parent.x + c->spec.x;
        out->y = parent.y + c->spec.y;
    }
    return 1;
}

static int host_ui_tab_is_active(int idx) {
    HostUiControl *c; int pi;
    if (idx < 0 || idx >= HOST_MAX_UI_CONTROLS || !g_host_ui_controls[idx].used) return 0;
    c=&g_host_ui_controls[idx];
    if(c->spec.kind!=NYOTA_UI_CTRL_TAB) return 1;
    pi=host_ui_control_index_by_handle(c->parent_control_handle);
    if(pi<0||g_host_ui_controls[pi].spec.kind!=NYOTA_UI_CTRL_TABS) return 0;
    return c->spec.tab_index==g_host_ui_controls[pi].spec.selected;
}

static int host_ui_control_visible_index(int idx) {
    int p=idx;
    while(p>=0){
        HostUiControl *c=&g_host_ui_controls[p];
        if(c->spec.kind==NYOTA_UI_CTRL_TAB && !host_ui_tab_is_active(p)) return 0;
        p=c->parent_control_handle>0?host_ui_control_index_by_handle(c->parent_control_handle):-1;
    }
    return 1;
}

static int host_ui_tab_header_rect(int idx, SDL_Rect *out) {
    HostUiControl *tab; int ti, pi, j, x, hh=30, scale, w; SDL_Rect pr;
    if(!out||idx<0||idx>=HOST_MAX_UI_CONTROLS)return 0;
    tab=&g_host_ui_controls[idx]; if(!tab->used||tab->spec.kind!=NYOTA_UI_CTRL_TAB)return 0;
    pi=host_ui_control_index_by_handle(tab->parent_control_handle);
    if(pi<0||g_host_ui_controls[pi].spec.kind!=NYOTA_UI_CTRL_TABS||!host_ui_control_rect_index(pi,&pr))return 0;
    if((int)tab->spec.tab_font_size+12>hh)hh=(int)tab->spec.tab_font_size+12;
    x=pr.x;
    for(j=0;j<HOST_MAX_UI_CONTROLS;j++){
        HostUiControl *q=&g_host_ui_controls[j];
        if(!q->used||q->spec.kind!=NYOTA_UI_CTRL_TAB||q->parent_control_handle!=tab->parent_control_handle||q->spec.tab_index>=tab->spec.tab_index)continue;
        scale=(int)((q->spec.tab_font_size+7u)/8u);if(scale<1)scale=1;
        x+=(int)strlen(q->spec.text)*8*scale+20;
    }
    scale=(int)((tab->spec.tab_font_size+7u)/8u);if(scale<1)scale=1;
    w=(int)strlen(tab->spec.text)*8*scale+20;if(w<48)w=48;
    out->x=x;out->y=pr.y;out->w=w;out->h=hh;
    ti=pr.x+pr.w; if(out->x+out->w>ti) out->w=ti-out->x;
    return out->w>0;
}

static int host_ui_point_in_control(int idx, int x, int y) {
    HostUiControl *c;
    SDL_Rect r;
    double nx, ny;
    int p;
    if (!host_ui_control_visible_index(idx) || !host_ui_control_rect_index(idx, &r)) return 0;
    if (x < r.x || y < r.y || x >= r.x + r.w || y >= r.y + r.h) return 0;
    c = &g_host_ui_controls[idx];
    if (c->spec.kind == NYOTA_UI_CTRL_DAREA) {
        if (c->spec.shape == NYOTA_UI_DAREA_CIRCLE) {
            double radius = (double)(r.w < r.h ? r.w : r.h) / 2.0;
            double cx = r.x + r.w / 2.0, cy = r.y + r.h / 2.0;
            double dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy > radius * radius) return 0;
        } else if (c->spec.shape == NYOTA_UI_DAREA_ELLIPSE) {
            if (r.w <= 0 || r.h <= 0) return 0;
            nx = ((double)x - (r.x + r.w / 2.0)) / ((double)r.w / 2.0);
            ny = ((double)y - (r.y + r.h / 2.0)) / ((double)r.h / 2.0);
            if (nx * nx + ny * ny > 1.0) return 0;
        }
    }
    p = c->parent_control_handle > 0 ? host_ui_control_index_by_handle(c->parent_control_handle) : -1;
    while (p >= 0) {
        HostUiControl *pc = &g_host_ui_controls[p];
        SDL_Rect pr;
        if (pc->spec.kind == NYOTA_UI_CTRL_PANEL && pc->spec.clip) {
            if (!host_ui_control_rect_index(p, &pr) ||
                x < pr.x || y < pr.y || x >= pr.x + pr.w || y >= pr.y + pr.h) return 0;
        }
        p = pc->parent_control_handle > 0 ? host_ui_control_index_by_handle(pc->parent_control_handle) : -1;
    }
    return 1;
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
                        host_ui_redraw_controls(ui);
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
        if (e.type == SDL_MOUSEMOTION) {
            int ui = host_ui_index_by_window_id(e.motion.windowID);
            if (ui >= 0) {
                int changed = 0, i;
                for (i = 0; i < HOST_MAX_UI_CONTROLS; i++) {
                    HostUiControl *ctl = &g_host_ui_controls[i];
                    if (!ctl->used || ctl->window_handle != g_ui_windows[ui].handle) continue;
                    if (ctl->spec.kind == NYOTA_UI_CTRL_DAREA) {
                        uint8_t over = (uint8_t)host_ui_point_in_control(i, e.motion.x, e.motion.y);
                        if (over != ctl->hover) { ctl->hover = over; changed = 1; }
                    } else if (ctl->spec.kind == NYOTA_UI_CTRL_COMBO && ctl->combo_open) {
                        SDL_Rect r;
                        int32_t hi=-1;
                        if(host_ui_control_rect_index(i,&r) && e.motion.x>=r.x && e.motion.x<r.x+r.w &&
                           e.motion.y>=r.y+r.h) {
                            int row=(e.motion.y-(r.y+r.h))/r.h;
                            uint32_t count=0,k; for(k=0;ctl->spec.items[k];k++)if(ctl->spec.items[k]=='\n')count++;
                            if(row>=0 && (uint32_t)row<count && (uint32_t)row<ctl->spec.max_visible) hi=row;
                        }
                        if(hi!=ctl->combo_hover){ctl->combo_hover=hi;changed=1;}
                    }
                }
                if (changed) {
                    if (g_ui_windows[ui].has_background) host_ui_render_background_index(ui);
                    host_ui_redraw_controls(ui);
                }
            }
            continue;
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int ui=host_ui_index_by_window_id(e.button.windowID);
            if(ui>=0){
                int i,changed=0;
                for(i=HOST_MAX_UI_CONTROLS-1;i>=0;i--){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    if(!ctl->used||ctl->window_handle!=g_ui_windows[ui].handle)continue;
                    if(ctl->spec.kind==NYOTA_UI_CTRL_TAB){
                        SDL_Rect hr;
                        if(host_ui_tab_header_rect(i,&hr)&&e.button.x>=hr.x&&e.button.x<hr.x+hr.w&&e.button.y>=hr.y&&e.button.y<hr.y+hr.h){
                            int pi=host_ui_control_index_by_handle(ctl->parent_control_handle);
                            if(pi>=0){g_host_ui_controls[pi].spec.selected=ctl->spec.tab_index;changed=1;} break;
                        }
                    }
                    if(ctl->spec.kind==NYOTA_UI_CTRL_COMBO){
                        SDL_Rect r;
                        if(!host_ui_control_rect_index(i,&r))continue;
                        if(ctl->combo_open && e.button.x>=r.x&&e.button.x<r.x+r.w&&e.button.y>=r.y+r.h){
                            int row=(e.button.y-(r.y+r.h))/r.h; uint32_t count=0,k;
                            for(k=0;ctl->spec.items[k];k++)if(ctl->spec.items[k]=='\n')count++;
                            if(row>=0&&(uint32_t)row<count&&(uint32_t)row<ctl->spec.max_visible)ctl->spec.selected=(uint32_t)row;
                            ctl->combo_open=0;ctl->combo_hover=-1;changed=1;break;
                        }
                        if(host_ui_point_in_control(i,e.button.x,e.button.y)){ctl->combo_open=(uint8_t)!ctl->combo_open;ctl->combo_hover=-1;changed=1;break;}
                    }
                    if((ctl->spec.kind==NYOTA_UI_CTRL_CBOX||ctl->spec.kind==NYOTA_UI_CTRL_RADIO)&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        if(ctl->spec.kind==NYOTA_UI_CTRL_CBOX)ctl->spec.checked=(uint8_t)!ctl->spec.checked;
                        else{
                            int j;ctl->spec.checked=1;
                            for(j=0;j<HOST_MAX_UI_CONTROLS;j++)if(j!=i&&g_host_ui_controls[j].used&&g_host_ui_controls[j].spec.kind==NYOTA_UI_CTRL_RADIO&&g_host_ui_controls[j].window_handle==ctl->window_handle&&ctl->spec.radio_group[0]&&!strcmp(g_host_ui_controls[j].spec.radio_group,ctl->spec.radio_group))g_host_ui_controls[j].spec.checked=0;
                        }
                        changed=1;break;
                    }
                }
                if(changed){if(g_ui_windows[ui].has_background)host_ui_render_background_index(ui);host_ui_redraw_controls(ui);}
            }
            continue;
        }
        if (e.type == SDL_DROPFILE) {
            int ui = host_ui_index_by_window_id(e.drop.windowID);
            if (ui >= 0 && e.drop.file) {
                int mx = 0, my = 0, i;
                struct stat st;
                SDL_GetMouseState(&mx, &my);
                for (i = HOST_MAX_UI_CONTROLS - 1; i >= 0; i--) {
                    HostUiControl *ctl = &g_host_ui_controls[i];
                    size_t cur, add;
                    int is_dir, accepted;
                    if (!ctl->used || ctl->window_handle != g_ui_windows[ui].handle ||
                        ctl->spec.kind != NYOTA_UI_CTRL_DAREA ||
                        !host_ui_point_in_control(i, mx, my)) continue;
                    if (stat(e.drop.file, &st) != 0) break;
                    is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
                    accepted = ctl->spec.accept == NYOTA_UI_ACCEPT_ALL ||
                               (is_dir && ctl->spec.accept == NYOTA_UI_ACCEPT_DIRS) ||
                               (!is_dir && ctl->spec.accept == NYOTA_UI_ACCEPT_FILES);
                    if (!accepted) break;
                    if (!ctl->spec.multi) ctl->drop_items[0] = '\0';
                    cur = strlen(ctl->drop_items); add = strlen(e.drop.file);
                    if (cur + add + 2 < sizeof(ctl->drop_items)) {
                        memcpy(ctl->drop_items + cur, e.drop.file, add);
                        cur += add; ctl->drop_items[cur++] = '\n'; ctl->drop_items[cur] = '\0';
                        ctl->dropped = 1;
                    }
                    break;
                }
                SDL_free(e.drop.file);
            }
            continue;
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
}


static int host_ui_shape_inside(const NyotaUiControlSpec *s, int x, int y, int w, int h) {
    if (!s || s->kind != NYOTA_UI_CTRL_DAREA || s->shape == NYOTA_UI_DAREA_RECT) return 1;
    if (s->shape == NYOTA_UI_DAREA_CIRCLE) {
        double radius = (double)(w < h ? w : h) / 2.0;
        double dx = (double)x + 0.5 - (double)w / 2.0;
        double dy = (double)y + 0.5 - (double)h / 2.0;
        return dx * dx + dy * dy <= radius * radius;
    }
    if (s->shape == NYOTA_UI_DAREA_ELLIPSE) {
        double nx, ny;
        if (w <= 0 || h <= 0) return 0;
        nx = ((double)x + 0.5 - (double)w / 2.0) / ((double)w / 2.0);
        ny = ((double)y + 0.5 - (double)h / 2.0) / ((double)h / 2.0);
        return nx * nx + ny * ny <= 1.0;
    }
    return 1;
}


static int host_ui_inside_rounded(int x,int y,int w,int h,uint32_t radius){
    int r=(int)radius; int dx,dy;
    if(r<=0)return 1;if(r>w/2)r=w/2;if(r>h/2)r=h/2;
    if(x>=r&&x<w-r)return 1;if(y>=r&&y<h-r)return 1;
    dx=x<r?(r-1-x):(x-(w-r)); dy=y<r?(r-1-y):(y-(h-r));
    return dx*dx+dy*dy<=r*r;
}
static SDL_Surface *host_ui_control_background_surface(const NyotaUiControlSpec *s,
                                                        const NyotaUiBackground *bg) {
    SDL_Surface *dst = NULL, *src = NULL, *conv = NULL;
    uint32_t x, y;
    if (!s || !bg || !s->w || !s->h) return NULL;
    dst = SDL_CreateRGBSurfaceWithFormat(0, (int)s->w, (int)s->h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!dst) return NULL;
    SDL_FillRect(dst, NULL, SDL_MapRGBA(dst->format, 0, 0, 0, 0));

    if (bg->kind == NYOTA_UI_BG_IMAGE) {
        SDL_Rect sr, dr;
        int sw, sh, dw, dh;
        src = IMG_Load(bg->image_path);
        if (!src) { SDL_FreeSurface(dst); return NULL; }
        conv = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(src); src = NULL;
        if (!conv) { SDL_FreeSurface(dst); return NULL; }
        sw = conv->w; sh = conv->h;
        sr.x = 0; sr.y = 0; sr.w = sw; sr.h = sh;
        dr.x = 0; dr.y = 0; dr.w = (int)s->w; dr.h = (int)s->h;
        if (bg->image_mode == NYOTA_UI_IMG_STRETCH) {
            SDL_BlitScaled(conv, NULL, dst, &dr);
        } else if (bg->image_mode == NYOTA_UI_IMG_NATIVE) {
            SDL_BlitSurface(conv, NULL, dst, &dr);
        } else if (bg->image_mode == NYOTA_UI_IMG_TILE) {
            int xx, yy;
            for (yy = 0; yy < (int)s->h; yy += sh)
                for (xx = 0; xx < (int)s->w; xx += sw) {
                    dr.x = xx; dr.y = yy; dr.w = sw; dr.h = sh;
                    SDL_BlitSurface(conv, NULL, dst, &dr);
                }
        } else if (bg->image_mode == NYOTA_UI_IMG_FIT) {
            if ((int64_t)sw * s->h > (int64_t)sh * s->w) {
                dw = (int)s->w; dh = (int)((int64_t)sh * s->w / sw);
            } else {
                dh = (int)s->h; dw = (int)((int64_t)sw * s->h / sh);
            }
            dr.w = dw; dr.h = dh; dr.x = ((int)s->w - dw) / 2; dr.y = ((int)s->h - dh) / 2;
            SDL_BlitScaled(conv, NULL, dst, &dr);
        } else {
            if ((int64_t)sw * s->h > (int64_t)sh * s->w) {
                sr.w = (int)((int64_t)sh * s->w / s->h); sr.x = (sw - sr.w) / 2;
            } else {
                sr.h = (int)((int64_t)sw * s->h / s->w); sr.y = (sh - sr.h) / 2;
            }
            SDL_BlitScaled(conv, &sr, dst, &dr);
        }
        SDL_FreeSurface(conv);
    } else {
        if (SDL_LockSurface(dst) != 0) { SDL_FreeSurface(dst); return NULL; }
        for (y = 0; y < s->h; y++) {
            uint32_t *row = (uint32_t *)((uint8_t *)dst->pixels + y * dst->pitch);
            for (x = 0; x < s->w; x++) {
                uint8_t r=0,g=0,b=0,a=0;
                if (bg->kind == NYOTA_UI_BG_LINEAR || bg->kind == NYOTA_UI_BG_SHAPE ||
                    bg->kind == NYOTA_UI_BG_SPIRAL) {
                    double t = host_ui_gradient_t(bg, x, y, s->w, s->h);
                    host_ui_lerp_color(bg, t, &r, &g, &b, &a);
                } else {
                    NyotaColor cc = bg->colors[0];
                    r=cc.r; g=cc.g; b=cc.b; a=(cc.mode == NYOTA_COLOR_TRANSPARENT ? 0 : cc.a);
                }
                row[x] = SDL_MapRGBA(dst->format, r, g, b, a);
            }
        }
        SDL_UnlockSurface(dst);
    }

    if ((s->kind == NYOTA_UI_CTRL_DAREA && s->shape != NYOTA_UI_DAREA_RECT) || s->kind == NYOTA_UI_CTRL_RADIO || s->radius) {
        if (SDL_LockSurface(dst) == 0) {
            for (y = 0; y < s->h; y++) {
                uint32_t *row = (uint32_t *)((uint8_t *)dst->pixels + y * dst->pitch);
                for (x = 0; x < s->w; x++) {
                    int inside=1;
                    if (s->kind == NYOTA_UI_CTRL_RADIO) {
                        double dx=(double)x+0.5-(double)s->w/2.0,dy=(double)y+0.5-(double)s->h/2.0;
                        double rr=(double)(s->w<s->h?s->w:s->h)/2.0; inside=dx*dx+dy*dy<=rr*rr;
                    } else if (s->kind == NYOTA_UI_CTRL_DAREA && s->shape != NYOTA_UI_DAREA_RECT) inside=host_ui_shape_inside(s,(int)x,(int)y,(int)s->w,(int)s->h);
                    else if (s->radius) inside=host_ui_inside_rounded((int)x,(int)y,(int)s->w,(int)s->h,s->radius);
                    if(!inside) row[x]=SDL_MapRGBA(dst->format,0,0,0,0);
                }
            }
            SDL_UnlockSurface(dst);
        }
    }
    return dst;
}

static void host_ui_draw_text(SDL_Renderer *ren, const NyotaUiControlSpec *s, SDL_Rect r) {
    const char *text;
    int scale, charw, charh, len, tx, ty, i, row, col;
    NyotaColor color;
    if (!ren || !s || !(text = s->text) || !text[0] || s->kind == NYOTA_UI_CTRL_PANEL) return;
    color = s->text_color;
    if (color.mode == NYOTA_COLOR_TRANSPARENT) return;
    scale = (int)((s->font_size + 7u) / 8u);
    if (scale < 1) scale = 1; if (scale > 16) scale = 16;
    charw = 8 * scale; charh = 8 * scale; len = (int)strlen(text);
    tx = r.x + 4;
    if (s->halign == NYOTA_UI_ALIGN_CENTER) tx = r.x + (r.w - len * charw) / 2;
    else if (s->halign == NYOTA_UI_ALIGN_RIGHT) tx = r.x + r.w - len * charw - 4;
    ty = r.y + 4;
    if (s->valign == NYOTA_UI_VALIGN_MIDDLE) ty = r.y + (r.h - charh) / 2;
    else if (s->valign == NYOTA_UI_VALIGN_BOTTOM) ty = r.y + r.h - charh - 4;
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
    for (i = 0; text[i]; i++) {
        unsigned ch = (unsigned char)text[i];
        const unsigned char *glyph;
        int gx = tx + i * charw;
        if (ch < 32 || ch > 126) ch = '?';
        glyph = FONT8[ch - 32];
        for (row = 0; row < 8; row++) {
            unsigned char bits = glyph[row];
            int slant = s->italic ? (7 - row) * scale / 3 : 0;
            for (col = 0; col < 8; col++) if (bits & (1u << col)) {
                SDL_Rect px = {gx + col * scale + slant, ty + row * scale, scale, scale};
                SDL_RenderFillRect(ren, &px);
                if (s->bold) { px.x += 1; SDL_RenderFillRect(ren, &px); }
            }
        }
    }
    if (s->underline) {
        SDL_RenderDrawLine(ren, tx, ty + charh - 1, tx + len * charw - 1, ty + charh - 1);
    }
}

static void host_ui_draw_rounded_rect(SDL_Renderer *ren, SDL_Rect q, int radius) {
    const double pi = 3.14159265358979323846;
    int seg, nseg = 12, px = 0, py = 0, first = 1;
    if (!ren || q.w <= 1 || q.h <= 1) return;
    if (radius <= 0) { SDL_RenderDrawRect(ren, &q); return; }
    if (radius > q.w / 2) radius = q.w / 2;
    if (radius > q.h / 2) radius = q.h / 2;
    for (seg = 0; seg <= nseg * 4; seg++) {
        int quad = seg / nseg;
        int step = seg % nseg;
        double a;
        double cx, cy;
        if (quad > 3) { quad = 3; step = nseg; }
        if (quad == 0) { a = pi + (pi / 2.0) * step / nseg; cx = q.x + radius; cy = q.y + radius; }
        else if (quad == 1) { a = 1.5 * pi + (pi / 2.0) * step / nseg; cx = q.x + q.w - 1 - radius; cy = q.y + radius; }
        else if (quad == 2) { a = (pi / 2.0) * step / nseg; cx = q.x + q.w - 1 - radius; cy = q.y + q.h - 1 - radius; }
        else { a = pi / 2.0 + (pi / 2.0) * step / nseg; cx = q.x + radius; cy = q.y + q.h - 1 - radius; }
        {
            int nx = (int)lrint(cx + cos(a) * radius);
            int ny = (int)lrint(cy + sin(a) * radius);
            if (!first) SDL_RenderDrawLine(ren, px, py, nx, ny);
            first = 0; px = nx; py = ny;
        }
    }
}

static void host_ui_draw_border(SDL_Renderer *ren, const HostUiControl *ctl, SDL_Rect r) {
    uint32_t k, width;
    NyotaColor color;
    if (!ren || !ctl || !ctl->spec.border) return;
    color = (ctl->spec.kind == NYOTA_UI_CTRL_DAREA && ctl->hover) ?
            ctl->spec.border_color_over : ctl->spec.border_color;
    width = (ctl->spec.kind == NYOTA_UI_CTRL_DAREA && ctl->hover) ?
            ctl->spec.border_width_over : ctl->spec.border_width;
    if (color.mode == NYOTA_COLOR_TRANSPARENT || !width) return;
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
    for (k = 0; k < width; k++) {
        SDL_Rect q = {r.x + (int)k, r.y + (int)k, r.w - (int)(2*k), r.h - (int)(2*k)};
        if (q.w <= 1 || q.h <= 1) break;
        if ((ctl->spec.kind == NYOTA_UI_CTRL_DAREA && ctl->spec.shape != NYOTA_UI_DAREA_RECT) || ctl->spec.kind == NYOTA_UI_CTRL_RADIO) {
            int seg, nseg = 96;
            double pi = 3.14159265358979323846;
            int px = 0, py = 0;
            double rx = (ctl->spec.kind == NYOTA_UI_CTRL_RADIO || ctl->spec.shape == NYOTA_UI_DAREA_CIRCLE ?
                        (double)(q.w < q.h ? q.w : q.h) / 2.0 : (double)q.w / 2.0);
            double ry = (ctl->spec.kind == NYOTA_UI_CTRL_RADIO || ctl->spec.shape == NYOTA_UI_DAREA_CIRCLE ? rx : (double)q.h / 2.0);
            double cx = q.x + q.w / 2.0, cy = q.y + q.h / 2.0;
            for (seg = 0; seg <= nseg; seg++) {
                double a = 2.0 * pi * seg / nseg;
                int nx = (int)lrint(cx + cos(a) * rx);
                int ny = (int)lrint(cy + sin(a) * ry);
                if (seg) SDL_RenderDrawLine(ren, px, py, nx, ny);
                px = nx; py = ny;
            }
        } else if (ctl->spec.radius) {
            int rr = (int)ctl->spec.radius - (int)k;
            if (rr < 0) rr = 0;
            host_ui_draw_rounded_rect(ren, q, rr);
        } else SDL_RenderDrawRect(ren, &q);
    }
}

static int host_ui_clip_for_control(int idx, SDL_Rect *clip) {
    int p;
    SDL_Rect full, pr, inter;
    HostUiControl *c;
    int wi;
    if (idx < 0 || idx >= HOST_MAX_UI_CONTROLS || !g_host_ui_controls[idx].used) return 0;
    c=&g_host_ui_controls[idx]; wi=host_ui_index_by_handle(c->window_handle); if(wi<0)return 0;
    full.x=0;full.y=0;full.w=(int)g_ui_windows[wi].w;full.h=(int)g_ui_windows[wi].h; *clip=full;
    p=c->parent_control_handle>0?host_ui_control_index_by_handle(c->parent_control_handle):-1;
    while(p>=0){
        HostUiControl *pc=&g_host_ui_controls[p];
        if((pc->spec.kind==NYOTA_UI_CTRL_PANEL||pc->spec.kind==NYOTA_UI_CTRL_TAB) && pc->spec.clip && host_ui_control_rect_index(p,&pr)){
            if(!SDL_IntersectRect(clip,&pr,&inter)){clip->w=clip->h=0;return 1;} *clip=inter;
        }
        p=pc->parent_control_handle>0?host_ui_control_index_by_handle(pc->parent_control_handle):-1;
    }
    return 1;
}

static void host_ui_draw_simple_text(SDL_Renderer *ren,const char *text,NyotaColor c,SDL_Rect r,uint32_t fs){
    NyotaUiControlSpec t; memset(&t,0,sizeof(t)); t.font_size=fs;t.text_color=c;t.halign=NYOTA_UI_ALIGN_LEFT;t.valign=NYOTA_UI_VALIGN_MIDDLE;strncpy(t.text,text,sizeof(t.text)-1);host_ui_draw_text(ren,&t,r);
}
static int host_ui_item_at(const char *items,uint32_t wanted,char *out,size_t cap){
    uint32_t cur=0,start=0,i=0;if(!out||!cap)return 0;out[0]='\0';
    while(1){char ch=items[i];if(ch=='\n'||ch=='\0'){if(cur==wanted){size_t n=i-start;if(n>=cap)n=cap-1;memcpy(out,items+start,n);out[n]='\0';return 1;}cur++;start=i+1;if(ch=='\0')break;}i++;}return 0;
}
static void host_ui_draw_shadow(SDL_Renderer *ren,const NyotaUiControlSpec *s,SDL_Rect r){
    int dx=0,dy=0,d=(int)s->shadow_depth;SDL_Rect q=r;
    if(!ren||s->shadow==NYOTA_UI_SHADOW_OFF||!d)return;
    if(s->shadow==NYOTA_UI_SHADOW_R||s->shadow==NYOTA_UI_SHADOW_RU||s->shadow==NYOTA_UI_SHADOW_RD)dx=d;
    if(s->shadow==NYOTA_UI_SHADOW_L||s->shadow==NYOTA_UI_SHADOW_LU||s->shadow==NYOTA_UI_SHADOW_LD)dx=-d;
    if(s->shadow==NYOTA_UI_SHADOW_U||s->shadow==NYOTA_UI_SHADOW_RU||s->shadow==NYOTA_UI_SHADOW_LU)dy=-d;
    if(s->shadow==NYOTA_UI_SHADOW_D||s->shadow==NYOTA_UI_SHADOW_RD||s->shadow==NYOTA_UI_SHADOW_LD)dy=d;
    q.x+=dx;q.y+=dy;SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);SDL_SetRenderDrawColor(ren,s->shadow_color.r,s->shadow_color.g,s->shadow_color.b,s->shadow_color.a);
    if(s->kind==NYOTA_UI_CTRL_SEP){if(s->orientation==NYOTA_UI_SEP_VERTICAL)q.w=(int)s->sep_thickness;else q.h=(int)s->sep_thickness;SDL_RenderFillRect(ren,&q);}
    else SDL_RenderFillRect(ren,&q);
}
static void host_ui_draw_tab_header(int idx,SDL_Renderer *ren){
    HostUiControl *c=&g_host_ui_controls[idx];SDL_Rect r;SDL_Surface *sf;SDL_Texture *tx;NyotaUiControlSpec tmp;
    if(!host_ui_tab_header_rect(idx,&r))return;
    memset(&tmp,0,sizeof(tmp));tmp.w=(uint32_t)r.w;tmp.h=(uint32_t)r.h;tmp.background=c->spec.tab_background;tmp.radius=c->spec.tab_radius;
    sf=host_ui_control_background_surface(&tmp,&tmp.background);if(sf){tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);if(tx){SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);SDL_RenderCopy(ren,tx,NULL,&r);SDL_DestroyTexture(tx);}}
    if(c->spec.tab_border&&c->spec.tab_border_color.mode!=NYOTA_COLOR_TRANSPARENT){
        int k;SDL_SetRenderDrawColor(ren,c->spec.tab_border_color.r,c->spec.tab_border_color.g,c->spec.tab_border_color.b,c->spec.tab_border_color.a);
        for(k=0;k<(int)c->spec.tab_border_width;k++){SDL_RenderDrawLine(ren,r.x+k,r.y+k,r.x+r.w-1-k,r.y+k);SDL_RenderDrawLine(ren,r.x+k,r.y+k,r.x+k,r.y+r.h-1);SDL_RenderDrawLine(ren,r.x+r.w-1-k,r.y+k,r.x+r.w-1-k,r.y+r.h-1);}
    }
    memset(&tmp,0,sizeof(tmp));tmp.font_size=c->spec.tab_font_size;tmp.text_color=c->spec.tab_text_color;tmp.bold=c->spec.tab_bold;tmp.italic=c->spec.tab_italic;tmp.underline=c->spec.tab_underline;tmp.halign=NYOTA_UI_ALIGN_CENTER;tmp.valign=NYOTA_UI_VALIGN_MIDDLE;strncpy(tmp.text,c->spec.text,sizeof(tmp.text)-1);host_ui_draw_text(ren,&tmp,r);
}
static void host_ui_draw_control_index(int idx) {
    HostUiControl *ctl; HostUiWindow *u; SDL_Rect r,clip; SDL_Surface *surface; SDL_Texture *tex; const NyotaUiBackground *bg; int wi;
    if(idx<0||idx>=HOST_MAX_UI_CONTROLS||!g_host_ui_controls[idx].used)return;ctl=&g_host_ui_controls[idx];wi=host_ui_index_by_handle(ctl->window_handle);
    if(wi<0||!(u=&g_ui_windows[wi])->ren)return;
    if(ctl->spec.kind==NYOTA_UI_CTRL_TAB) host_ui_draw_tab_header(idx,u->ren);
    if(!host_ui_control_visible_index(idx))return;
    if(!host_ui_control_rect_index(idx,&r)||!host_ui_clip_for_control(idx,&clip)||clip.w<=0||clip.h<=0)return;
    SDL_RenderSetClipRect(u->ren,&clip);
    if(ctl->spec.kind==NYOTA_UI_CTRL_TABS){SDL_RenderSetClipRect(u->ren,NULL);return;}
    host_ui_draw_shadow(u->ren,&ctl->spec,r);
    if(ctl->spec.kind==NYOTA_UI_CTRL_SEP){
        uint32_t t=ctl->spec.sep_thickness;NyotaColor c=ctl->spec.sep_color;SDL_Rect q=r;
        if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL)q.w=(int)t;else q.h=(int)t;
        if(ctl->spec.sep_effect==NYOTA_UI_SEP_GRADIENT){
            NyotaUiControlSpec ts=ctl->spec;ts.w=(uint32_t)q.w;ts.h=(uint32_t)q.h;surface=host_ui_control_background_surface(&ts,&ctl->spec.background);
            if(surface){tex=SDL_CreateTextureFromSurface(u->ren,surface);SDL_FreeSurface(surface);if(tex){SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);SDL_RenderCopy(u->ren,tex,NULL,&q);SDL_DestroyTexture(tex);}}
        } else {
            SDL_SetRenderDrawColor(u->ren,c.r,c.g,c.b,c.a);SDL_RenderFillRect(u->ren,&q);
            if(ctl->spec.sep_effect==NYOTA_UI_SEP_INSET||ctl->spec.sep_effect==NYOTA_UI_SEP_RAISED){
                NyotaColor hi=c,lo=c;hi.r=(uint8_t)(hi.r+(255-hi.r)/2);hi.g=(uint8_t)(hi.g+(255-hi.g)/2);hi.b=(uint8_t)(hi.b+(255-hi.b)/2);lo.r/=2;lo.g/=2;lo.b/=2;
                NyotaColor a=ctl->spec.sep_effect==NYOTA_UI_SEP_INSET?lo:hi,b=ctl->spec.sep_effect==NYOTA_UI_SEP_INSET?hi:lo;
                SDL_SetRenderDrawColor(u->ren,a.r,a.g,a.b,a.a);if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL)SDL_RenderDrawLine(u->ren,q.x,q.y,q.x,q.y+q.h-1);else SDL_RenderDrawLine(u->ren,q.x,q.y,q.x+q.w-1,q.y);
                SDL_SetRenderDrawColor(u->ren,b.r,b.g,b.b,b.a);if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL)SDL_RenderDrawLine(u->ren,q.x+q.w-1,q.y,q.x+q.w-1,q.y+q.h-1);else SDL_RenderDrawLine(u->ren,q.x,q.y+q.h-1,q.x+q.w-1,q.y+q.h-1);
            }
        }
        SDL_RenderSetClipRect(u->ren,NULL);return;
    }
    bg=(ctl->spec.kind==NYOTA_UI_CTRL_DAREA&&ctl->hover)?&ctl->spec.background_over:&ctl->spec.background;
    surface=host_ui_control_background_surface(&ctl->spec,bg);if(surface){tex=SDL_CreateTextureFromSurface(u->ren,surface);SDL_FreeSurface(surface);if(tex){SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);SDL_RenderCopy(u->ren,tex,NULL,&r);SDL_DestroyTexture(tex);}}
    host_ui_draw_border(u->ren,ctl,r);
    if(ctl->spec.kind==NYOTA_UI_CTRL_CBOX&&ctl->spec.checked){
        SDL_Rect tr=r;host_ui_draw_simple_text(u->ren,ctl->spec.check_symbol,ctl->spec.check_color,tr,ctl->spec.w>8?ctl->spec.w-4:8);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_RADIO&&ctl->spec.checked){
        int rr=(r.w<r.h?r.w:r.h)/4;SDL_Rect dot={r.x+r.w/2-rr,r.y+r.h/2-rr,rr*2,rr*2};SDL_SetRenderDrawColor(u->ren,ctl->spec.check_color.r,ctl->spec.check_color.g,ctl->spec.check_color.b,ctl->spec.check_color.a);
        for(int yy=-rr;yy<=rr;yy++){int xx=(int)sqrt((double)(rr*rr-yy*yy));SDL_RenderDrawLine(u->ren,r.x+r.w/2-xx,r.y+r.h/2+yy,r.x+r.w/2+xx,r.y+r.h/2+yy);}
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_COMBO){
        char item[NYOTA_UI_TEXT_MAX];SDL_Rect txr=r;txr.w-=r.h;
        if(host_ui_item_at(ctl->spec.items,ctl->spec.selected,item,sizeof(item)))host_ui_draw_simple_text(u->ren,item,ctl->spec.text_color,txr,ctl->spec.font_size);
        SDL_SetRenderDrawColor(u->ren,ctl->spec.arrow_color.r,ctl->spec.arrow_color.g,ctl->spec.arrow_color.b,ctl->spec.arrow_color.a);int cx=r.x+r.w-r.h/2,cy=r.y+r.h/2;SDL_RenderDrawLine(u->ren,cx-5,cy-2,cx,cy+3);SDL_RenderDrawLine(u->ren,cx,cy+3,cx+5,cy-2);
        if(ctl->combo_open){
            uint32_t count=0,k,vis;for(k=0;ctl->spec.items[k];k++)if(ctl->spec.items[k]=='\n')count++;vis=count<ctl->spec.max_visible?count:ctl->spec.max_visible;
            for(k=0;k<vis;k++){SDL_Rect row={r.x,r.y+r.h+(int)k*r.h,r.w,r.h};const NyotaUiBackground *rb=&ctl->spec.drop_background;NyotaColor tc=ctl->spec.drop_text_color;if(k==ctl->spec.selected){rb=&ctl->spec.selected_background;tc=ctl->spec.selected_text_color;}if(ctl->spec.hover_enabled&&(int32_t)k==ctl->combo_hover){rb=&ctl->spec.hover_background;tc=ctl->spec.hover_text_color;}NyotaUiControlSpec rs=ctl->spec;rs.w=(uint32_t)row.w;rs.h=(uint32_t)row.h;surface=host_ui_control_background_surface(&rs,rb);if(surface){tex=SDL_CreateTextureFromSurface(u->ren,surface);SDL_FreeSurface(surface);if(tex){SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);SDL_RenderCopy(u->ren,tex,NULL,&row);SDL_DestroyTexture(tex);}}if(host_ui_item_at(ctl->spec.items,k,item,sizeof(item)))host_ui_draw_simple_text(u->ren,item,tc,row,ctl->spec.font_size);}
        }
    } else host_ui_draw_text(u->ren,&ctl->spec,r);
    SDL_RenderSetClipRect(u->ren,NULL);
}

static void host_ui_redraw_controls(int window_index) {
    int i;
    HostUiWindow *u;
    if(window_index<0||window_index>=HOST_MAX_UI_WINDOWS||!g_ui_windows[window_index].used)return;
    u=&g_ui_windows[window_index];
    for(i=0;i<HOST_MAX_UI_CONTROLS;i++)
        if(g_host_ui_controls[i].used&&g_host_ui_controls[i].window_handle==u->handle)
            host_ui_draw_control_index(i);
    if(u->ren) SDL_RenderPresent(u->ren);
}

static int32_t host_ui_control_create(const char *name, int32_t window_handle,
                                      int32_t parent_control_handle,
                                      const NyotaUiControlSpec *spec) {
    int i, wi = host_ui_index_by_handle(window_handle);
    (void)name;
    if (wi < 0 || !spec || !spec->w || !spec->h) return -1;
    if (parent_control_handle > 0) {
        int pi = host_ui_control_index_by_handle(parent_control_handle);
        if (pi < 0 || g_host_ui_controls[pi].window_handle != window_handle) return -1;
        if (spec->kind == NYOTA_UI_CTRL_TAB) {
            if (g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_TABS) return -1;
        } else if (g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_PANEL &&
                   g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_TAB) return -1;
    }
    for(i=0;i<HOST_MAX_UI_CONTROLS;i++) if(!g_host_ui_controls[i].used){
        memset(&g_host_ui_controls[i],0,sizeof(g_host_ui_controls[i]));
        g_host_ui_controls[i].used=1; g_host_ui_controls[i].handle=1000+i;
        g_host_ui_controls[i].window_handle=window_handle;
        g_host_ui_controls[i].parent_control_handle=parent_control_handle;
        g_host_ui_controls[i].spec=*spec;
        g_host_ui_controls[i].combo_hover=-1;
        if (g_ui_windows[wi].has_background) host_ui_render_background_index(wi);
        host_ui_redraw_controls(wi);
        return g_host_ui_controls[i].handle;
    }
    return -1;
}

static int32_t host_ui_control_update(int32_t handle, const NyotaUiControlSpec *spec) {
    int i=host_ui_control_index_by_handle(handle), wi;
    if(i<0||!spec)return -1;
    g_host_ui_controls[i].spec=*spec;
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);
    if(wi<0)return -1;
    if(g_ui_windows[wi].has_background) host_ui_render_background_index(wi);
    host_ui_redraw_controls(wi);
    return 0;
}

static void host_ui_control_destroy_index(int idx) {
    int i, wi;
    int32_t handle;
    if(idx<0||idx>=HOST_MAX_UI_CONTROLS||!g_host_ui_controls[idx].used)return;
    handle=g_host_ui_controls[idx].handle;
    wi=host_ui_index_by_handle(g_host_ui_controls[idx].window_handle);
    for(i=0;i<HOST_MAX_UI_CONTROLS;i++)
        if(g_host_ui_controls[i].used&&g_host_ui_controls[i].parent_control_handle==handle)
            host_ui_control_destroy_index(i);
    memset(&g_host_ui_controls[idx],0,sizeof(g_host_ui_controls[idx]));
    if(wi>=0){if(g_ui_windows[wi].has_background)host_ui_render_background_index(wi);host_ui_redraw_controls(wi);}
}

static void host_ui_control_destroy(int32_t handle) {
    int i=host_ui_control_index_by_handle(handle); if(i>=0)host_ui_control_destroy_index(i);
}

static int32_t host_ui_button_clicked(int32_t handle) {
    int i=host_ui_control_index_by_handle(handle), wi, mx=0,my=0;
    Uint32 buttons; uint8_t down; int inside, clicked;
    if(i<0||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_BUTTON)return -1;
    host_pump();
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle); if(wi<0)return -1;
    if(SDL_GetMouseFocus()!=g_ui_windows[wi].win){g_host_ui_controls[i].prev_down=0;return 0;}
    buttons=SDL_GetMouseState(&mx,&my); down=(buttons&SDL_BUTTON(SDL_BUTTON_LEFT))?1:0;
    inside=host_ui_point_in_control(i,mx,my);
    clicked=down&&!g_host_ui_controls[i].prev_down&&inside;
    g_host_ui_controls[i].prev_down=down;
    return clicked?1:0;
}

static int32_t host_ui_darea_dropped(int32_t handle) {
    int i=host_ui_control_index_by_handle(handle); int32_t v;
    if(i<0||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_DAREA)return -1;
    host_pump(); v=g_host_ui_controls[i].dropped?1:0; g_host_ui_controls[i].dropped=0; return v;
}

static int32_t host_ui_darea_items(int32_t handle,char *out,uint32_t cap,uint32_t *out_size){
    int i=host_ui_control_index_by_handle(handle); size_t n;
    if(out_size)*out_size=0;
    if(i<0||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_DAREA||!out||!cap)return -1;
    n=strlen(g_host_ui_controls[i].drop_items); if(n+1>cap)return -2;
    memcpy(out,g_host_ui_controls[i].drop_items,n+1); if(out_size)*out_size=(uint32_t)n; return 0;
}

static int32_t host_ui_control_checked(int32_t handle){
    int i=host_ui_control_index_by_handle(handle);if(i<0)return -1;
    if(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_CBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_RADIO)return -1;
    host_pump();return g_host_ui_controls[i].spec.checked?1:0;
}
static int32_t host_ui_control_set_checked(int32_t handle,uint8_t checked){
    int i=host_ui_control_index_by_handle(handle),j,wi;if(i<0)return -1;
    if(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_CBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_RADIO)return -1;
    g_host_ui_controls[i].spec.checked=checked?1:0;
    if(checked&&g_host_ui_controls[i].spec.kind==NYOTA_UI_CTRL_RADIO&&g_host_ui_controls[i].spec.radio_group[0])
        for(j=0;j<HOST_MAX_UI_CONTROLS;j++)if(j!=i&&g_host_ui_controls[j].used&&g_host_ui_controls[j].spec.kind==NYOTA_UI_CTRL_RADIO&&g_host_ui_controls[j].window_handle==g_host_ui_controls[i].window_handle&&!strcmp(g_host_ui_controls[j].spec.radio_group,g_host_ui_controls[i].spec.radio_group))g_host_ui_controls[j].spec.checked=0;
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);if(wi>=0){if(g_ui_windows[wi].has_background)host_ui_render_background_index(wi);host_ui_redraw_controls(wi);}return 0;
}
static int32_t host_ui_combo_index(int32_t handle){int i=host_ui_control_index_by_handle(handle);if(i<0||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_COMBO)return -1;host_pump();return (int32_t)g_host_ui_controls[i].spec.selected;}
static int32_t host_ui_combo_set_index(int32_t handle,uint32_t index){int i=host_ui_control_index_by_handle(handle),wi;uint32_t count=0,k;if(i<0||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_COMBO)return -1;for(k=0;g_host_ui_controls[i].spec.items[k];k++)if(g_host_ui_controls[i].spec.items[k]=='\n')count++;if(index>=count)return -1;g_host_ui_controls[i].spec.selected=index;wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);if(wi>=0){if(g_ui_windows[wi].has_background)host_ui_render_background_index(wi);host_ui_redraw_controls(wi);}return 0;}


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
    host_ui_render_background_index(i);
    host_ui_redraw_controls(i);
    return 0;
}

static void host_ui_destroy_index(int idx) {
    int i;
    int32_t handle;
    if (idx < 0 || idx >= HOST_MAX_UI_WINDOWS || !g_ui_windows[idx].used) return;
    handle = g_ui_windows[idx].handle;
    for (i = 0; i < HOST_MAX_UI_CONTROLS; i++)
        if (g_host_ui_controls[i].used && g_host_ui_controls[i].window_handle == handle)
            memset(&g_host_ui_controls[i], 0, sizeof(g_host_ui_controls[i]));
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
    g_nyhost.ui_control_create = host_ui_control_create;
    g_nyhost.ui_control_update = host_ui_control_update;
    g_nyhost.ui_control_destroy = host_ui_control_destroy;
    g_nyhost.ui_button_clicked = host_ui_button_clicked;
    g_nyhost.ui_darea_dropped = host_ui_darea_dropped;
    g_nyhost.ui_darea_items = host_ui_darea_items;
    g_nyhost.ui_control_checked = host_ui_control_checked;
    g_nyhost.ui_control_set_checked = host_ui_control_set_checked;
    g_nyhost.ui_combo_index = host_ui_combo_index;
    g_nyhost.ui_combo_set_index = host_ui_combo_set_index;
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
