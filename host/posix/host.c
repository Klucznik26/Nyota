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
#include <SDL2/SDL_ttf.h>
#include <fontconfig/fontconfig.h>
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
#define HOST_MAX_UI_FONTS 32
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
    SDL_Texture *background_cache;
    int has_background;
    uint8_t dirty;
    uint8_t shown;
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
    uint8_t pressed;
    uint8_t focused;
    uint8_t clicked;
    uint8_t dropped;
    uint8_t combo_open;
    int32_t combo_hover;
    uint8_t range_dragging;
    int32_t range_drag_offset;
    uint8_t scale_dragging;
    uint8_t changed;
    int32_t list_hover;
    uint64_t list_selected_mask;
    uint64_t tree_expanded_mask;
    uint32_t caret;
    uint8_t text_changed;
    uint32_t eq_peak_value[NYOTA_UI_EQ_MAX_BARS];
    uint64_t eq_peak_tick[NYOTA_UI_EQ_MAX_BARS];
    SDL_Texture *eq_barbg_cache;
    SDL_Texture *eq_fill_cache;
    SDL_Texture *eq_glow_cache;
    uint32_t eq_cache_w;
    uint32_t eq_cache_h;
    SDL_Texture *icon_cache;
    SDL_Texture *tree_leaf_cache;
    SDL_Texture *tree_closed_cache;
    SDL_Texture *tree_open_cache;
    char drop_items[NYOTA_UI_DROP_MAX];
} HostUiControl;

static HostUiControl g_host_ui_controls[HOST_MAX_UI_CONTROLS];

typedef struct {
    int used;
    char family[NYOTA_UI_FONT_MAX];
    int size;
    int style;
    TTF_Font *font;
} HostUiFont;

static HostUiFont g_host_ui_fonts[HOST_MAX_UI_FONTS];

static void host_ui_render_background_index(int idx);
static void host_ui_destroy_index(int idx);
static void host_ui_redraw_controls(int window_index);
static void host_ui_mark_dirty(int window_index);
static void host_ui_flush_dirty(void);
static void host_ui_finish_initial_build(void);
static void host_ui_clear_eq_cache(HostUiControl *ctl);
static void host_ui_clear_asset_cache(HostUiControl *ctl);
static int host_ui_combo_popup_hit(int idx,int x,int y,int32_t *row_out);
static uint32_t host_ui_combo_item_count(const HostUiControl *ctl);
static int host_ui_item_at(const char *items,uint32_t wanted,char *out,size_t cap);
static NyotaColor host_ui_tint_color(NyotaColor c,int delta);
static uint32_t host_ui_tarea_caret_from_point(HostUiControl *ctl,SDL_Rect r,int mx,int my);
static void host_ui_tarea_caret_visual(HostUiControl *ctl,SDL_Rect r,int *out_x,int *out_row);
static int host_ui_splitter_bar_rect(int idx,SDL_Rect *bar);
static uint32_t host_ui_splitter_value_from_pointer(int idx,int x,int y);
static int32_t host_ui_scale_value_from_pointer(int idx,int mx,int my);
static int host_ui_tree_visible_row_to_index(const HostUiControl *ctl,uint32_t row,uint32_t *out);
static int host_ui_tree_has_child(const HostUiControl *ctl,uint32_t index);
static int host_ui_tree_item_visible(const HostUiControl *ctl,uint32_t index);
static uint32_t host_ui_tree_depth_text(const char *s);
static int host_ui_input_edit_rect(int idx,SDL_Rect full,SDL_Rect *edit);
static int g_ui_draw_clip_idx = -1;
static uint8_t g_ui_initializing = 1;

static void posix_emit(char c) {
    fputc(c, stdout);
    if (c == '\n') fflush(stdout);
}

static uint64_t host_ticks(void);

static int host_ui_resolve_font_path(const char *family, char *out, size_t cap) {
    FcPattern *pat = NULL, *match = NULL;
    FcResult result;
    FcChar8 *file = NULL;
    const char *requested = family && family[0] && strcmp(family, "SYSTEM") ? family : "sans-serif";
    if (!out || cap == 0) return 0;
    out[0] = '\0';
    pat = FcPatternCreate();
    if (!pat) return 0;
    FcPatternAddString(pat, FC_FAMILY, (const FcChar8 *)requested);
    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    match = FcFontMatch(NULL, pat, &result);
    FcPatternDestroy(pat);
    if (!match) return 0;
    if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file) {
        strncpy(out, (const char *)file, cap - 1);
        out[cap - 1] = '\0';
    }
    FcPatternDestroy(match);
    return out[0] != '\0';
}

static TTF_Font *host_ui_get_font(const char *family, uint32_t size, uint8_t bold, uint8_t italic, uint8_t underline) {
    int i, style = TTF_STYLE_NORMAL;
    char path[1024];
    TTF_Font *font;
    if (!size) size = 14;
    if (size < 8) size = 8;
    if (size > 128) size = 128;
    if (bold) style |= TTF_STYLE_BOLD;
    if (italic) style |= TTF_STYLE_ITALIC;
    if (underline) style |= TTF_STYLE_UNDERLINE;
    for (i = 0; i < HOST_MAX_UI_FONTS; i++) {
        if (g_host_ui_fonts[i].used &&
            g_host_ui_fonts[i].size == (int)size &&
            g_host_ui_fonts[i].style == style &&
            !strcmp(g_host_ui_fonts[i].family, family && family[0] ? family : "SYSTEM"))
            return g_host_ui_fonts[i].font;
    }
    if (!host_ui_resolve_font_path(family, path, sizeof(path))) return NULL;
    font = TTF_OpenFont(path, (int)size);
    if (!font) return NULL;
    TTF_SetFontStyle(font, style);
    TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
    for (i = 0; i < HOST_MAX_UI_FONTS; i++) {
        if (!g_host_ui_fonts[i].used) {
            g_host_ui_fonts[i].used = 1;
            g_host_ui_fonts[i].size = (int)size;
            g_host_ui_fonts[i].style = style;
            strncpy(g_host_ui_fonts[i].family, family && family[0] ? family : "SYSTEM",
                    sizeof(g_host_ui_fonts[i].family) - 1);
            g_host_ui_fonts[i].family[sizeof(g_host_ui_fonts[i].family) - 1] = '\0';
            g_host_ui_fonts[i].font = font;
            return font;
        }
    }
    TTF_CloseFont(font);
    return NULL;
}

static int host_ui_measure_text(const char *family, const char *text, uint32_t size,
                                uint8_t bold, uint8_t italic, uint8_t underline,
                                int *w, int *h) {
    TTF_Font *font;
    if (w) *w = 0;
    if (h) *h = 0;
    if (!text || !text[0]) return 0;
    font = host_ui_get_font(family, size, bold, italic, underline);
    if (!font) return 0;
    return TTF_SizeUTF8(font, text, w, h) == 0;
}

static SDL_Texture *host_ui_texture_from_path(SDL_Renderer *ren,SDL_Texture **cache,const char *path){
    SDL_Surface *sf;SDL_Texture *tx;
    if(!ren||!cache||!path||!path[0])return NULL;
    if(*cache)return *cache;
    sf=IMG_Load(path);if(!sf)return NULL;
    tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);
    if(!tx)return NULL;
    SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);*cache=tx;return tx;
}
static void host_ui_draw_texture_fit(SDL_Renderer *ren,SDL_Texture *tx,SDL_Rect box,uint8_t alpha){
    int w=0,h=0,dw,dh;SDL_Rect d;
    if(!ren||!tx||box.w<=0||box.h<=0||SDL_QueryTexture(tx,NULL,NULL,&w,&h)!=0||w<=0||h<=0)return;
    if((int64_t)box.w*h<=(int64_t)box.h*w){dw=box.w;dh=(int)((int64_t)h*dw/w);}
    else{dh=box.h;dw=(int)((int64_t)w*dh/h);}
    if(dw<1)dw=1;if(dh<1)dh=1;
    d.x=box.x+(box.w-dw)/2;d.y=box.y+(box.h-dh)/2;d.w=dw;d.h=dh;
    SDL_SetTextureAlphaMod(tx,alpha);SDL_RenderCopy(ren,tx,NULL,&d);SDL_SetTextureAlphaMod(tx,255);
}

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
    } else if (c->spec.position_mode == NYOTA_UI_POS_AUTO) {
        int pi = c->parent_control_handle > 0 ? host_ui_control_index_by_handle(c->parent_control_handle) : -1;
        int j;
        int xoff = 0, yoff = 0;
        if (pi < 0 || (g_host_ui_controls[pi].spec.layout != NYOTA_UI_LAYOUT_ROW &&
                       g_host_ui_controls[pi].spec.layout != NYOTA_UI_LAYOUT_COL)) return 0;
        xoff = (int)g_host_ui_controls[pi].spec.layout_pad_x;
        yoff = (int)g_host_ui_controls[pi].spec.layout_pad_y;
        for (j = 0; j < idx; j++) {
            HostUiControl *q = &g_host_ui_controls[j];
            if (!q->used || q->parent_control_handle != c->parent_control_handle ||
                q->spec.position_mode != NYOTA_UI_POS_AUTO) continue;
            if (g_host_ui_controls[pi].spec.layout == NYOTA_UI_LAYOUT_ROW)
                xoff += (int)q->spec.w + (int)g_host_ui_controls[pi].spec.gap;
            else
                yoff += (int)q->spec.h + (int)g_host_ui_controls[pi].spec.gap;
        }
        out->x = parent.x + xoff;
        out->y = parent.y + yoff;
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
    HostUiControl *tab; int ti, pi, j, x, hh=30, w, tw=0, th=0; SDL_Rect pr;
    if(!out||idx<0||idx>=HOST_MAX_UI_CONTROLS)return 0;
    tab=&g_host_ui_controls[idx]; if(!tab->used||tab->spec.kind!=NYOTA_UI_CTRL_TAB)return 0;
    pi=host_ui_control_index_by_handle(tab->parent_control_handle);
    if(pi<0||g_host_ui_controls[pi].spec.kind!=NYOTA_UI_CTRL_TABS||!host_ui_control_rect_index(pi,&pr))return 0;
    if(host_ui_measure_text(tab->spec.tab_font, tab->spec.text, tab->spec.tab_font_size,
                            tab->spec.tab_bold, tab->spec.tab_italic, tab->spec.tab_underline, &tw, &th)) {
        if(th+(int)(2u*tab->spec.tab_pad_y)>hh) hh=th+(int)(2u*tab->spec.tab_pad_y);
    } else if((int)tab->spec.tab_font_size+(int)(2u*tab->spec.tab_pad_y)>hh) hh=(int)tab->spec.tab_font_size+(int)(2u*tab->spec.tab_pad_y);
    x=pr.x;
    for(j=0;j<HOST_MAX_UI_CONTROLS;j++){
        HostUiControl *q=&g_host_ui_controls[j];
        int qw=0,qh=0;
        if(!q->used||q->spec.kind!=NYOTA_UI_CTRL_TAB||q->parent_control_handle!=tab->parent_control_handle||q->spec.tab_index>=tab->spec.tab_index)continue;
        if(!host_ui_measure_text(q->spec.tab_font, q->spec.text, q->spec.tab_font_size,
                                 q->spec.tab_bold, q->spec.tab_italic, q->spec.tab_underline, &qw, &qh))
            qw=(int)strlen(q->spec.text)*(int)q->spec.tab_font_size;
        x+=qw+(int)(2u*q->spec.tab_pad_x);
    }
    w=tw>0?tw+(int)(2u*tab->spec.tab_pad_x):(int)strlen(tab->spec.text)*(int)tab->spec.tab_font_size+(int)(2u*tab->spec.tab_pad_x);
    if(w<52)w=52;
    out->x=x;out->y=pr.y;out->w=w;out->h=hh;
    ti=pr.x+pr.w; if(out->x+out->w>ti) out->w=ti-out->x;
    return out->w>0;
}

static int host_ui_control_focusable(const HostUiControl *ctl) {
    if (!ctl || !ctl->used || !ctl->spec.enabled) return 0;
    return ctl->spec.kind == NYOTA_UI_CTRL_BUTTON ||
           ctl->spec.kind == NYOTA_UI_CTRL_CBOX ||
           ctl->spec.kind == NYOTA_UI_CTRL_RADIO ||
           ctl->spec.kind == NYOTA_UI_CTRL_COMBO ||
           ctl->spec.kind == NYOTA_UI_CTRL_DAREA ||
           ctl->spec.kind == NYOTA_UI_CTRL_TAB ||
           ctl->spec.kind == NYOTA_UI_CTRL_TAREA ||
           ctl->spec.kind == NYOTA_UI_CTRL_TBOX ||
           ctl->spec.kind == NYOTA_UI_CTRL_SBAR ||
           ctl->spec.kind == NYOTA_UI_CTRL_SLIDER ||
           ctl->spec.kind == NYOTA_UI_CTRL_SPINBOX ||
           ctl->spec.kind == NYOTA_UI_CTRL_LISTVIEW ||
           ctl->spec.kind == NYOTA_UI_CTRL_TREEVIEW ||
           ctl->spec.kind == NYOTA_UI_CTRL_SPLITTER ||
           ctl->spec.kind == NYOTA_UI_CTRL_SCALE ||
           ctl->spec.kind == NYOTA_UI_CTRL_ICONBUTTON ||
           ctl->spec.kind == NYOTA_UI_CTRL_SWITCH ||
           ctl->spec.kind == NYOTA_UI_CTRL_INPUT;
}

static void host_ui_set_focus(int window_index, int wanted) {
    int i, text_focus = 0;
    if (window_index < 0 || window_index >= HOST_MAX_UI_WINDOWS || !g_ui_windows[window_index].used) return;
    for (i = 0; i < HOST_MAX_UI_CONTROLS; i++) {
        HostUiControl *ctl = &g_host_ui_controls[i];
        if (!ctl->used || ctl->window_handle != g_ui_windows[window_index].handle) continue;
        ctl->focused = (i == wanted && host_ui_control_focusable(ctl)) ? 1 : 0;
        if (ctl->focused && (ctl->spec.kind == NYOTA_UI_CTRL_TAREA || ctl->spec.kind == NYOTA_UI_CTRL_TBOX || ctl->spec.kind == NYOTA_UI_CTRL_INPUT) && ctl->spec.enabled && !ctl->spec.readonly)
            text_focus = 1;
    }
    if (text_focus) SDL_StartTextInput(); else SDL_StopTextInput();
}

static void host_ui_focus_next(int window_index, int reverse) {
    int i, current = -1, first = -1, last = -1, next = -1;
    int32_t wh;
    if (window_index < 0 || window_index >= HOST_MAX_UI_WINDOWS || !g_ui_windows[window_index].used) return;
    wh = g_ui_windows[window_index].handle;
    for (i = 0; i < HOST_MAX_UI_CONTROLS; i++) {
        HostUiControl *ctl = &g_host_ui_controls[i];
        if (!ctl->used || ctl->window_handle != wh || !host_ui_control_visible_index(i) ||
            !host_ui_control_focusable(ctl)) continue;
        if (first < 0) first = i;
        last = i;
        if (ctl->focused) current = i;
    }
    if (first < 0) return;
    if (current < 0) next = reverse ? last : first;
    else if (reverse) {
        for (i = current - 1; i >= 0; i--) {
            HostUiControl *ctl = &g_host_ui_controls[i];
            if (ctl->used && ctl->window_handle == wh && host_ui_control_visible_index(i) &&
                host_ui_control_focusable(ctl)) { next = i; break; }
        }
        if (next < 0) next = last;
    } else {
        for (i = current + 1; i < HOST_MAX_UI_CONTROLS; i++) {
            HostUiControl *ctl = &g_host_ui_controls[i];
            if (ctl->used && ctl->window_handle == wh && host_ui_control_visible_index(i) &&
                host_ui_control_focusable(ctl)) { next = i; break; }
        }
        if (next < 0) next = first;
    }
    host_ui_set_focus(window_index, next);
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
        if ((pc->spec.kind == NYOTA_UI_CTRL_PANEL || pc->spec.kind == NYOTA_UI_CTRL_TAB || pc->spec.kind == NYOTA_UI_CTRL_STATBAR || pc->spec.kind == NYOTA_UI_CTRL_TOOLBAR || pc->spec.kind == NYOTA_UI_CTRL_FRAME) && pc->spec.clip) {
            if (!host_ui_control_rect_index(p, &pr) ||
                x < pr.x || y < pr.y || x >= pr.x + pr.w || y >= pr.y + pr.h) return 0;
        }
        p = pc->parent_control_handle > 0 ? host_ui_control_index_by_handle(pc->parent_control_handle) : -1;
    }
    return 1;
}

static uint32_t host_ui_range_clamp(const NyotaUiControlSpec *s,int64_t v){
    if(!s)return 0;
    if(v<(int64_t)s->range_min)return s->range_min;
    if(v>(int64_t)s->range_max)return s->range_max;
    return (uint32_t)v;
}
static int32_t host_ui_wrap_signed(int32_t v,int32_t minv,int32_t maxv){
    int64_t span=(int64_t)maxv-(int64_t)minv,x;
    if(span<=0)return minv;
    x=((int64_t)v-(int64_t)minv)%span;if(x<0)x+=span;
    return (int32_t)((int64_t)minv+x);
}
static int32_t host_ui_signed_clamp(const NyotaUiControlSpec *s,int64_t v){
    if(!s)return 0;
    if(s->kind==NYOTA_UI_CTRL_SCALE&&s->scale_wrap)return host_ui_wrap_signed((int32_t)v,s->signed_min,s->signed_max);
    if(v<(int64_t)s->signed_min)return s->signed_min;
    if(v>(int64_t)s->signed_max)return s->signed_max;
    return (int32_t)v;
}

static int host_ui_range_thumb_rect(int idx,SDL_Rect *out){
    HostUiControl *ctl;
    SDL_Rect r;
    uint32_t span,page;
    int track,cross,len,avail,pos;
    if(!out||idx<0||idx>=HOST_MAX_UI_CONTROLS)return 0;
    ctl=&g_host_ui_controls[idx];
    if(!ctl->used||(ctl->spec.kind!=NYOTA_UI_CTRL_SBAR&&ctl->spec.kind!=NYOTA_UI_CTRL_SLIDER)||
       !host_ui_control_rect_index(idx,&r)||ctl->spec.range_max<=ctl->spec.range_min)return 0;
    if(ctl->spec.border&&ctl->spec.border_width){
        int p=(int)ctl->spec.border_width;
        r.x+=p;r.y+=p;r.w-=2*p;r.h-=2*p;
        if(r.w<=0||r.h<=0)return 0;
    }
    span=ctl->spec.range_max-ctl->spec.range_min;
    track=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?r.h:r.w;
    cross=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?r.w:r.h;
    if(track<=0||cross<=0)return 0;
    if(ctl->spec.kind==NYOTA_UI_CTRL_SLIDER){
        len=(int)ctl->spec.slider_thumb_size;
        if(len<4)len=4;
    }else{
        page=ctl->spec.range_page?ctl->spec.range_page:1;
        len=(int)(((uint64_t)track*(uint64_t)page)/((uint64_t)span+(uint64_t)page));
        if(len<cross)len=cross;
        if(len<8)len=8;
    }
    if(len>track)len=track;
    avail=track-len;
    pos=avail>0?(int)(((uint64_t)(ctl->spec.range_value-ctl->spec.range_min)*(uint64_t)avail)/(uint64_t)span):0;
    if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){
        out->x=r.x;out->y=r.y+pos;out->w=r.w;out->h=len;
    }else{
        out->x=r.x+pos;out->y=r.y;out->w=len;out->h=r.h;
    }
    return 1;
}

static uint32_t host_ui_range_value_from_pointer(int idx,int x,int y){
    HostUiControl *ctl=&g_host_ui_controls[idx];
    SDL_Rect r,thumb;
    int track,len,avail,p;
    uint32_t span;
    if(!host_ui_control_rect_index(idx,&r)||!host_ui_range_thumb_rect(idx,&thumb)||
       ctl->spec.range_max<=ctl->spec.range_min)return ctl->spec.range_value;
    if(ctl->spec.border&&ctl->spec.border_width){
        int pad=(int)ctl->spec.border_width;
        r.x+=pad;r.y+=pad;r.w-=2*pad;r.h-=2*pad;
        if(r.w<=0||r.h<=0)return ctl->spec.range_value;
    }
    span=ctl->spec.range_max-ctl->spec.range_min;
    track=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?r.h:r.w;
    len=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?thumb.h:thumb.w;
    avail=track-len;
    if(avail<=0)return ctl->spec.range_min;
    p=(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?y-r.y:x-r.x)-ctl->range_drag_offset;
    if(p<0)p=0;if(p>avail)p=avail;
    return ctl->spec.range_min+(uint32_t)(((uint64_t)p*(uint64_t)span+(uint64_t)avail/2u)/(uint64_t)avail);
}

static uint32_t host_ui_utf8_prev(const char *s,uint32_t pos){
    if(!s||!pos)return 0;
    pos--;
    while(pos>0&&(((unsigned char)s[pos]&0xC0u)==0x80u))pos--;
    return pos;
}
static uint32_t host_ui_utf8_next(const char *s,uint32_t pos){
    uint32_t n;
    if(!s)return pos;
    n=(uint32_t)strlen(s);
    if(pos>=n)return n;
    pos++;
    while(pos<n&&(((unsigned char)s[pos]&0xC0u)==0x80u))pos++;
    return pos;
}
static int host_ui_input_partial_valid(const HostUiControl *ctl,const char *s){
    const char *p;
    if(!ctl||!s)return 0;
    if(ctl->spec.kind!=NYOTA_UI_CTRL_INPUT)return 1;
    if(ctl->spec.input_type==NYOTA_UI_INPUT_NUMBER){
        p=s;if(*p=='-'||*p=='+')p++;if(!*p)return 1;
        while(*p){if(*p<'0'||*p>'9')return 0;p++;}return 1;
    }
    if(ctl->spec.input_type==NYOTA_UI_INPUT_EMAIL){
        p=s;while(*p){if(*p==' '||*p=='\t'||*p=='\n'||*p=='\r')return 0;p++;}return 1;
    }
    return strchr(s,'\n')==NULL&&strchr(s,'\r')==NULL;
}
static int host_ui_tarea_insert(HostUiControl *ctl,const char *text){
    size_t len,add,limit;
    if(!ctl||!text||(ctl->spec.kind!=NYOTA_UI_CTRL_TAREA&&ctl->spec.kind!=NYOTA_UI_CTRL_TBOX&&ctl->spec.kind!=NYOTA_UI_CTRL_INPUT)||ctl->spec.readonly)return 0;
    if((ctl->spec.kind==NYOTA_UI_CTRL_TBOX||ctl->spec.kind==NYOTA_UI_CTRL_INPUT)&&(strchr(text,'\n')||strchr(text,'\r')))return 0;
    len=strlen(ctl->spec.text);add=strlen(text);limit=(ctl->spec.kind==NYOTA_UI_CTRL_TBOX||ctl->spec.kind==NYOTA_UI_CTRL_INPUT)?ctl->spec.max_length:sizeof(ctl->spec.text)-1;
    if(len+add>limit||len+add>=sizeof(ctl->spec.text))return 0;
    if(ctl->caret>len)ctl->caret=(uint32_t)len;
    if(ctl->spec.kind==NYOTA_UI_CTRL_INPUT){
        char tmp[NYOTA_UI_TEXT_MAX];
        if(len+add>=sizeof(tmp))return 0;
        memcpy(tmp,ctl->spec.text,ctl->caret);
        memcpy(tmp+ctl->caret,text,add);
        memcpy(tmp+ctl->caret+add,ctl->spec.text+ctl->caret,len-ctl->caret+1);
        if(!host_ui_input_partial_valid(ctl,tmp))return 0;
    }
    memmove(ctl->spec.text+ctl->caret+add,ctl->spec.text+ctl->caret,len-ctl->caret+1);
    memcpy(ctl->spec.text+ctl->caret,text,add);
    ctl->caret+=(uint32_t)add;ctl->text_changed=1;return 1;
}
static int host_ui_tarea_backspace(HostUiControl *ctl){
    uint32_t p;size_t len;
    if(!ctl||(ctl->spec.kind!=NYOTA_UI_CTRL_TAREA&&ctl->spec.kind!=NYOTA_UI_CTRL_TBOX&&ctl->spec.kind!=NYOTA_UI_CTRL_INPUT)||ctl->spec.readonly||!ctl->caret)return 0;
    len=strlen(ctl->spec.text);if(ctl->caret>len)ctl->caret=(uint32_t)len;
    p=host_ui_utf8_prev(ctl->spec.text,ctl->caret);
    memmove(ctl->spec.text+p,ctl->spec.text+ctl->caret,len-ctl->caret+1);
    ctl->caret=p;ctl->text_changed=1;return 1;
}
static int host_ui_tarea_delete(HostUiControl *ctl){
    uint32_t n;size_t len;
    if(!ctl||(ctl->spec.kind!=NYOTA_UI_CTRL_TAREA&&ctl->spec.kind!=NYOTA_UI_CTRL_TBOX&&ctl->spec.kind!=NYOTA_UI_CTRL_INPUT)||ctl->spec.readonly)return 0;
    len=strlen(ctl->spec.text);if(ctl->caret>=len)return 0;
    n=host_ui_utf8_next(ctl->spec.text,ctl->caret);
    memmove(ctl->spec.text+ctl->caret,ctl->spec.text+n,len-n+1);
    ctl->text_changed=1;return 1;
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
                        if(g_ui_windows[ui].background_cache) {
                            SDL_DestroyTexture(g_ui_windows[ui].background_cache);
                            g_ui_windows[ui].background_cache = NULL;
                        }
                        if(g_ui_windows[ui].ren)SDL_RenderSetLogicalSize(g_ui_windows[ui].ren,w,h);
                        host_ui_mark_dirty(ui);
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
                int changed = 0, i, popup_owner = -1;

                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *dc=&g_host_ui_controls[i];
                    if(!dc->used||dc->window_handle!=g_ui_windows[ui].handle||!dc->spec.enabled)continue;
                    if((dc->spec.kind==NYOTA_UI_CTRL_SBAR||dc->spec.kind==NYOTA_UI_CTRL_SLIDER)&&dc->range_dragging){
                        uint32_t nv=host_ui_range_value_from_pointer(i,e.motion.x,e.motion.y);
                        if(nv!=dc->spec.range_value){dc->spec.range_value=nv;dc->changed=1;changed=1;}
                    }else if(dc->spec.kind==NYOTA_UI_CTRL_SPLITTER&&dc->range_dragging){
                        uint32_t nv=host_ui_splitter_value_from_pointer(i,e.motion.x,e.motion.y);
                        if(nv!=dc->spec.range_value){dc->spec.range_value=nv;dc->changed=1;changed=1;}
                    }else if(dc->spec.kind==NYOTA_UI_CTRL_SCALE&&dc->scale_dragging&&dc->spec.scale_interactive){
                        int32_t nv=host_ui_scale_value_from_pointer(i,e.motion.x,e.motion.y);
                        if(nv!=dc->spec.signed_value){dc->spec.signed_value=nv;dc->changed=1;changed=1;}
                    }
                }

                for (i = HOST_MAX_UI_CONTROLS - 1; i >= 0; i--) {
                    int32_t row = -1;
                    HostUiControl *pc = &g_host_ui_controls[i];
                    if (!pc->used || pc->window_handle != g_ui_windows[ui].handle ||
                        pc->spec.kind != NYOTA_UI_CTRL_COMBO || !pc->combo_open) continue;
                    if (host_ui_combo_popup_hit(i, e.motion.x, e.motion.y, &row)) {
                        popup_owner = i;
                        break;
                    }
                }
                for (i = 0; i < HOST_MAX_UI_CONTROLS; i++) {
                    HostUiControl *ctl = &g_host_ui_controls[i];
                    uint8_t over = 0;
                    if (!ctl->used || ctl->window_handle != g_ui_windows[ui].handle) continue;
                    if (!ctl->spec.enabled) {
                        if (ctl->hover) { ctl->hover = 0; changed = 1; }
                        continue;
                    }

                    if (ctl->spec.kind == NYOTA_UI_CTRL_TAB) {
                        SDL_Rect hr;
                        over = (uint8_t)(host_ui_tab_header_rect(i,&hr) &&
                               e.motion.x>=hr.x && e.motion.x<hr.x+hr.w &&
                               e.motion.y>=hr.y && e.motion.y<hr.y+hr.h);
                    } else if (ctl->spec.kind == NYOTA_UI_CTRL_BUTTON ||
                               ctl->spec.kind == NYOTA_UI_CTRL_CBOX ||
                               ctl->spec.kind == NYOTA_UI_CTRL_RADIO ||
                               ctl->spec.kind == NYOTA_UI_CTRL_COMBO ||
                               ctl->spec.kind == NYOTA_UI_CTRL_DAREA ||
                               ctl->spec.kind == NYOTA_UI_CTRL_TAREA ||
                               ctl->spec.kind == NYOTA_UI_CTRL_TBOX ||
                               ctl->spec.kind == NYOTA_UI_CTRL_SBAR ||
                               ctl->spec.kind == NYOTA_UI_CTRL_SLIDER ||
                               ctl->spec.kind == NYOTA_UI_CTRL_SPINBOX ||
                               ctl->spec.kind == NYOTA_UI_CTRL_LISTVIEW ||
                               ctl->spec.kind == NYOTA_UI_CTRL_TREEVIEW ||
                               ctl->spec.kind == NYOTA_UI_CTRL_SPLITTER ||
                               ctl->spec.kind == NYOTA_UI_CTRL_SCALE ||
                               ctl->spec.kind == NYOTA_UI_CTRL_ICONBUTTON ||
                               ctl->spec.kind == NYOTA_UI_CTRL_SWITCH ||
                               ctl->spec.kind == NYOTA_UI_CTRL_INPUT) {
                        over = (uint8_t)host_ui_point_in_control(i,e.motion.x,e.motion.y);
                    }
                    if (popup_owner >= 0 && i != popup_owner) over = 0;
                    if (over != ctl->hover) { ctl->hover = over; changed = 1; }

                    if (ctl->spec.kind == NYOTA_UI_CTRL_COMBO && ctl->combo_open) {
                        SDL_Rect r;
                        int32_t hi=-1;
                        if(host_ui_control_rect_index(i,&r) && e.motion.x>=r.x && e.motion.x<r.x+r.w &&
                           e.motion.y>=r.y+r.h) {
                            int row=(e.motion.y-(r.y+r.h))/r.h;
                            uint32_t count=0,k;
                            for(k=0;ctl->spec.items[k];k++)if(ctl->spec.items[k]=='\n')count++;
                            if(row>=0 && (uint32_t)row<count && (uint32_t)row<ctl->spec.max_visible) hi=row;
                        }
                        if(hi!=ctl->combo_hover){ctl->combo_hover=hi;changed=1;}
                    }
                    if(ctl->spec.kind==NYOTA_UI_CTRL_LISTVIEW||ctl->spec.kind==NYOTA_UI_CTRL_TREEVIEW){
                        SDL_Rect lr;int32_t hi=-1;
                        if(host_ui_control_rect_index(i,&lr)&&e.motion.x>=lr.x&&e.motion.x<lr.x+lr.w&&e.motion.y>=lr.y&&e.motion.y<lr.y+lr.h){
                            int row=(e.motion.y-lr.y-1)/(int)(ctl->spec.row_height?ctl->spec.row_height:28);
                            if(row>=0){
                                if(ctl->spec.kind==NYOTA_UI_CTRL_TREEVIEW){uint32_t ix;if(host_ui_tree_visible_row_to_index(ctl,(uint32_t)row,&ix))hi=(int32_t)ix;}
                                else if((uint32_t)row<host_ui_combo_item_count(ctl))hi=row;
                            }
                        }
                        if(hi!=ctl->list_hover){ctl->list_hover=hi;changed=1;}
                    }
                }
                if (changed) host_ui_mark_dirty(ui);
            }
            continue;
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int ui=host_ui_index_by_window_id(e.button.windowID);
            if(ui>=0){
                int i,changed=0,popup_consumed=0,base_open_combo=-1;

                /* Popup COMBO ma pierwszenstwo przed wszystkimi kontrolkami pod nim. */
                for(i=HOST_MAX_UI_CONTROLS-1;i>=0;i--){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    int32_t row=-1;
                    if(!ctl->used||ctl->window_handle!=g_ui_windows[ui].handle||
                       ctl->spec.kind!=NYOTA_UI_CTRL_COMBO||!ctl->combo_open)continue;
                    if(host_ui_combo_popup_hit(i,e.button.x,e.button.y,&row)){
                        if(row>=0)ctl->spec.selected=(uint32_t)row;
                        ctl->combo_open=0;
                        ctl->combo_hover=-1;
                        host_ui_set_focus(ui,i);
                        changed=1;
                        popup_consumed=1;
                        break;
                    }
                }
                if(popup_consumed){
                    host_ui_mark_dirty(ui);
                    continue;
                }

                /* Klik poza popupem zamyka inne listy. Klik w baze otwartego
                   COMBO zostawiamy dla zwyklego toggle ponizej. */
                for(i=HOST_MAX_UI_CONTROLS-1;i>=0;i--){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    if(!ctl->used||ctl->window_handle!=g_ui_windows[ui].handle||
                       ctl->spec.kind!=NYOTA_UI_CTRL_COMBO||!ctl->combo_open)continue;
                    if(host_ui_point_in_control(i,e.button.x,e.button.y)){
                        base_open_combo=i;
                        break;
                    }
                }
                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    if(!ctl->used||ctl->window_handle!=g_ui_windows[ui].handle||
                       ctl->spec.kind!=NYOTA_UI_CTRL_COMBO||!ctl->combo_open||i==base_open_combo)continue;
                    ctl->combo_open=0;
                    ctl->combo_hover=-1;
                    changed=1;
                }

                for(i=HOST_MAX_UI_CONTROLS-1;i>=0;i--){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    int hit=0;
                    if(!ctl->used||ctl->window_handle!=g_ui_windows[ui].handle)continue;
                    if(!ctl->spec.enabled)continue;
                    if(ctl->spec.kind==NYOTA_UI_CTRL_TAB){
                        SDL_Rect hr;
                        hit=host_ui_tab_header_rect(i,&hr)&&e.button.x>=hr.x&&e.button.x<hr.x+hr.w&&e.button.y>=hr.y&&e.button.y<hr.y+hr.h;
                        if(hit){
                            int pi=host_ui_control_index_by_handle(ctl->parent_control_handle);
                            host_ui_set_focus(ui,i);
                            ctl->pressed=1;
                            if(pi>=0)g_host_ui_controls[pi].spec.selected=ctl->spec.tab_index;
                            changed=1;break;
                        }
                    }
                    if(ctl->spec.kind==NYOTA_UI_CTRL_COMBO){
                        SDL_Rect r;
                        if(!host_ui_control_rect_index(i,&r))continue;
                        if(host_ui_point_in_control(i,e.button.x,e.button.y)){
                            int j;
                            host_ui_set_focus(ui,i);
                            for(j=0;j<HOST_MAX_UI_CONTROLS;j++)
                                if(j!=i&&g_host_ui_controls[j].used&&
                                   g_host_ui_controls[j].window_handle==ctl->window_handle&&
                                   g_host_ui_controls[j].spec.kind==NYOTA_UI_CTRL_COMBO){
                                    g_host_ui_controls[j].combo_open=0;
                                    g_host_ui_controls[j].combo_hover=-1;
                                }
                            ctl->pressed=1;
                            ctl->combo_open=(uint8_t)!ctl->combo_open;
                            ctl->combo_hover=-1;
                            changed=1;break;
                        }
                    }
                    if((ctl->spec.kind==NYOTA_UI_CTRL_SBAR||ctl->spec.kind==NYOTA_UI_CTRL_SLIDER)&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        SDL_Rect th;
                        host_ui_set_focus(ui,i);
                        ctl->pressed=1;
                        if(host_ui_range_thumb_rect(i,&th)){
                            int along=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?e.button.y:e.button.x;
                            int start=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?th.y:th.x;
                            int len=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?th.h:th.w;
                            int inside=e.button.x>=th.x&&e.button.x<th.x+th.w&&e.button.y>=th.y&&e.button.y<th.y+th.h;
                            ctl->range_drag_offset=inside?along-start:len/2;
                            ctl->range_dragging=1;
                            if(!inside)ctl->spec.range_value=host_ui_range_value_from_pointer(i,e.button.x,e.button.y);
                        }
                        changed=1;break;
                    }
                    if(ctl->spec.kind==NYOTA_UI_CTRL_SPINBOX&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        SDL_Rect sr;int32_t nv=ctl->spec.signed_value;host_ui_set_focus(ui,i);
                        if(host_ui_control_rect_index(i,&sr)){
                            int bw=sr.h>30?30:sr.h;
                            if(e.button.x>=sr.x+sr.w-bw){
                                if(e.button.y<sr.y+sr.h/2)nv=host_ui_signed_clamp(&ctl->spec,(int64_t)nv+ctl->spec.signed_step);
                                else nv=host_ui_signed_clamp(&ctl->spec,(int64_t)nv-ctl->spec.signed_step);
                                if(nv!=ctl->spec.signed_value){ctl->spec.signed_value=nv;ctl->changed=1;}
                            }
                        }
                        ctl->pressed=1;changed=1;break;
                    }
                    if((ctl->spec.kind==NYOTA_UI_CTRL_LISTVIEW||ctl->spec.kind==NYOTA_UI_CTRL_TREEVIEW)&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        SDL_Rect lr;uint32_t ix=0;int row;host_ui_set_focus(ui,i);
                        if(host_ui_control_rect_index(i,&lr)){
                            row=(e.button.y-lr.y-1)/(int)(ctl->spec.row_height?ctl->spec.row_height:28);
                            if(row>=0&&((ctl->spec.kind==NYOTA_UI_CTRL_TREEVIEW&&host_ui_tree_visible_row_to_index(ctl,(uint32_t)row,&ix))||
                               (ctl->spec.kind==NYOTA_UI_CTRL_LISTVIEW&&(ix=(uint32_t)row)<host_ui_combo_item_count(ctl)))){
                                if(ctl->spec.kind==NYOTA_UI_CTRL_TREEVIEW&&host_ui_tree_has_child(ctl,ix)){
                                    char it[NYOTA_UI_TEXT_MAX];uint32_t depth=0;
                                    if(host_ui_item_at(ctl->spec.items,ix,it,sizeof(it))){
                                        depth=host_ui_tree_depth_text(it);
                                        if(e.button.x<lr.x+18+(int)(depth*ctl->spec.tree_indent)){
                                            if(ix<64)ctl->tree_expanded_mask^=(1ULL<<ix);
                                            changed=1;break;
                                        }
                                    }
                                }
                                if(ctl->spec.multi&&ix<64)ctl->list_selected_mask^=(1ULL<<ix);
                                else ctl->list_selected_mask=ix<64?(1ULL<<ix):0;
                                if(ctl->spec.selected!=ix){ctl->spec.selected=ix;ctl->changed=1;}
                                else if(ctl->spec.multi)ctl->changed=1;
                                changed=1;
                            }
                        }
                        break;
                    }
                    if(ctl->spec.kind==NYOTA_UI_CTRL_SPLITTER&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        uint32_t nv;host_ui_set_focus(ui,i);ctl->range_dragging=1;ctl->pressed=1;
                        nv=host_ui_splitter_value_from_pointer(i,e.button.x,e.button.y);
                        if(nv!=ctl->spec.range_value){ctl->spec.range_value=nv;ctl->changed=1;}
                        changed=1;break;
                    }
                    if(ctl->spec.kind==NYOTA_UI_CTRL_SCALE&&ctl->spec.scale_interactive&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        int32_t nv;host_ui_set_focus(ui,i);ctl->scale_dragging=1;ctl->pressed=1;
                        nv=host_ui_scale_value_from_pointer(i,e.button.x,e.button.y);
                        if(nv!=ctl->spec.signed_value){ctl->spec.signed_value=nv;ctl->changed=1;}
                        changed=1;break;
                    }
                    if((ctl->spec.kind==NYOTA_UI_CTRL_CBOX||ctl->spec.kind==NYOTA_UI_CTRL_RADIO)&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        host_ui_set_focus(ui,i);
                        ctl->pressed=1;
                        if(ctl->spec.kind==NYOTA_UI_CTRL_CBOX)ctl->spec.checked=(uint8_t)!ctl->spec.checked;
                        else{
                            int j;ctl->spec.checked=1;
                            for(j=0;j<HOST_MAX_UI_CONTROLS;j++)if(j!=i&&g_host_ui_controls[j].used&&g_host_ui_controls[j].spec.kind==NYOTA_UI_CTRL_RADIO&&g_host_ui_controls[j].window_handle==ctl->window_handle&&ctl->spec.radio_group[0]&&!strcmp(g_host_ui_controls[j].spec.radio_group,ctl->spec.radio_group))g_host_ui_controls[j].spec.checked=0;
                        }
                        changed=1;break;
                    }
                    if((ctl->spec.kind==NYOTA_UI_CTRL_TAREA||ctl->spec.kind==NYOTA_UI_CTRL_TBOX||ctl->spec.kind==NYOTA_UI_CTRL_INPUT)&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        SDL_Rect tr;
                        host_ui_set_focus(ui,i);
                        if(host_ui_control_rect_index(i,&tr)){
                            if(ctl->spec.kind==NYOTA_UI_CTRL_INPUT&&ctl->spec.clear_button&&e.button.x>=tr.x+tr.w-tr.h){
                                if(ctl->spec.text[0]){ctl->spec.text[0]='\0';ctl->caret=0;ctl->text_changed=1;}
                            }else ctl->caret=host_ui_tarea_caret_from_point(ctl,tr,e.button.x,e.button.y);
                        }else ctl->caret=(uint32_t)strlen(ctl->spec.text);
                        changed=1;break;
                    }
                    if(ctl->spec.kind==NYOTA_UI_CTRL_SWITCH&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        host_ui_set_focus(ui,i);ctl->pressed=1;ctl->spec.checked=(uint8_t)!ctl->spec.checked;ctl->changed=1;changed=1;break;
                    }
                    if((ctl->spec.kind==NYOTA_UI_CTRL_BUTTON||ctl->spec.kind==NYOTA_UI_CTRL_ICONBUTTON)&&host_ui_point_in_control(i,e.button.x,e.button.y)){
                        host_ui_set_focus(ui,i);
                        ctl->pressed=1;ctl->clicked=1;
                        if(ctl->spec.kind==NYOTA_UI_CTRL_ICONBUTTON&&ctl->spec.toggle){ctl->spec.checked=(uint8_t)!ctl->spec.checked;ctl->changed=1;}
                        changed=1;break;
                    }
                }
                if(changed)host_ui_mark_dirty(ui);
            }
            continue;
        }
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            int ui=host_ui_index_by_window_id(e.button.windowID);
            if(ui>=0){
                int i,changed=0;
                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    if(ctl->used&&ctl->window_handle==g_ui_windows[ui].handle){
                        if(ctl->pressed){ctl->pressed=0;changed=1;}
                        if(ctl->range_dragging){ctl->range_dragging=0;changed=1;}
                        if(ctl->scale_dragging){ctl->scale_dragging=0;changed=1;}
                    }
                }
                if(changed)host_ui_mark_dirty(ui);
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
                        !ctl->spec.enabled ||
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
        if (e.type == SDL_TEXTINPUT) {
            int ui=host_ui_index_by_window_id(e.text.windowID);
            if(ui>=0){
                int i,changed=0;
                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    if(ctl->used&&ctl->window_handle==g_ui_windows[ui].handle&&ctl->focused&&
                       (ctl->spec.kind==NYOTA_UI_CTRL_TAREA||ctl->spec.kind==NYOTA_UI_CTRL_TBOX||ctl->spec.kind==NYOTA_UI_CTRL_INPUT)&&ctl->spec.enabled&&!ctl->spec.readonly){
                        changed=host_ui_tarea_insert(ctl,e.text.text);break;
                    }
                }
                if(changed)host_ui_mark_dirty(ui);
            }
            continue;
        }
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            int ui = host_ui_index_by_window_id(e.key.windowID);
            if (k == SDLK_LSHIFT || k == SDLK_RSHIFT) g_shift = 1;
            if(ui>=0 && k!=SDLK_TAB){
                int i,handled=0,changed=0;

                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *sc=&g_host_ui_controls[i];
                    int64_t nv;
                    if(!sc->used||sc->window_handle!=g_ui_windows[ui].handle||!sc->focused||
                       (sc->spec.kind!=NYOTA_UI_CTRL_SBAR&&sc->spec.kind!=NYOTA_UI_CTRL_SLIDER&&sc->spec.kind!=NYOTA_UI_CTRL_SPLITTER)||!sc->spec.enabled)continue;
                    nv=sc->spec.range_value;
                    if(k==SDLK_LEFT||k==SDLK_UP)nv-=(int64_t)sc->spec.range_step;
                    else if(k==SDLK_RIGHT||k==SDLK_DOWN)nv+=(int64_t)sc->spec.range_step;
                    else if(k==SDLK_PAGEUP&&sc->spec.kind!=NYOTA_UI_CTRL_SPLITTER)nv-=(int64_t)sc->spec.range_page;
                    else if(k==SDLK_PAGEDOWN&&sc->spec.kind!=NYOTA_UI_CTRL_SPLITTER)nv+=(int64_t)sc->spec.range_page;
                    else if(k==SDLK_HOME)nv=sc->spec.range_min;
                    else if(k==SDLK_END)nv=sc->spec.range_max;
                    else break;
                    {
                        uint32_t cv=host_ui_range_clamp(&sc->spec,nv);
                        if(cv!=sc->spec.range_value){sc->spec.range_value=cv;if(sc->spec.kind==NYOTA_UI_CTRL_SPLITTER)sc->changed=1;changed=1;}
                    }
                    handled=1;
                    if(changed)host_ui_mark_dirty(ui);
                    break;
                }
                if(handled)continue;

                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *sc=&g_host_ui_controls[i];int64_t nv;int32_t cv;
                    if(!sc->used||sc->window_handle!=g_ui_windows[ui].handle||!sc->focused||
                       (sc->spec.kind!=NYOTA_UI_CTRL_SPINBOX&&sc->spec.kind!=NYOTA_UI_CTRL_SCALE)||!sc->spec.enabled)continue;
                    nv=sc->spec.signed_value;
                    if(k==SDLK_LEFT||k==SDLK_DOWN)nv-=(int64_t)sc->spec.signed_step;
                    else if(k==SDLK_RIGHT||k==SDLK_UP)nv+=(int64_t)sc->spec.signed_step;
                    else if(k==SDLK_HOME)nv=sc->spec.signed_min;
                    else if(k==SDLK_END)nv=sc->spec.kind==NYOTA_UI_CTRL_SCALE&&sc->spec.scale_wrap?sc->spec.signed_max-sc->spec.signed_step:sc->spec.signed_max;
                    else break;
                    cv=host_ui_signed_clamp(&sc->spec,nv);
                    if(cv!=sc->spec.signed_value){sc->spec.signed_value=cv;sc->changed=1;changed=1;}
                    handled=1;if(changed)host_ui_mark_dirty(ui);break;
                }
                if(handled)continue;

                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *lc=&g_host_ui_controls[i];uint32_t count,ix;
                    if(!lc->used||lc->window_handle!=g_ui_windows[ui].handle||!lc->focused||
                       (lc->spec.kind!=NYOTA_UI_CTRL_LISTVIEW&&lc->spec.kind!=NYOTA_UI_CTRL_TREEVIEW)||!lc->spec.enabled)continue;
                    count=host_ui_combo_item_count(lc);ix=lc->spec.selected;
                    if(k==SDLK_UP){
                        if(lc->spec.kind==NYOTA_UI_CTRL_TREEVIEW){
                            uint32_t j,prev=ix;for(j=0;j<ix;j++)if(host_ui_tree_item_visible(lc,j))prev=j;if(prev!=ix){lc->spec.selected=prev;lc->changed=1;changed=1;}
                        }else if(ix>0){lc->spec.selected=ix-1;lc->changed=1;changed=1;}
                        handled=1;
                    }else if(k==SDLK_DOWN){
                        if(lc->spec.kind==NYOTA_UI_CTRL_TREEVIEW){
                            uint32_t j;for(j=ix+1;j<count;j++)if(host_ui_tree_item_visible(lc,j)){lc->spec.selected=j;lc->changed=1;changed=1;break;}
                        }else if(ix+1<count){lc->spec.selected=ix+1;lc->changed=1;changed=1;}
                        handled=1;
                    }else if(lc->spec.kind==NYOTA_UI_CTRL_TREEVIEW&&(k==SDLK_LEFT||k==SDLK_RIGHT)){
                        if(ix<64&&host_ui_tree_has_child(lc,ix)){
                            uint64_t bit=1ULL<<ix;
                            if(k==SDLK_LEFT)lc->tree_expanded_mask&=~bit;else lc->tree_expanded_mask|=bit;
                            changed=1;handled=1;
                        }
                    }else if(k==SDLK_HOME&&count){lc->spec.selected=0;lc->changed=1;changed=handled=1;}
                    else if(k==SDLK_END&&count){
                        if(lc->spec.kind==NYOTA_UI_CTRL_TREEVIEW){uint32_t j,last=0;for(j=0;j<count;j++)if(host_ui_tree_item_visible(lc,j))last=j;lc->spec.selected=last;}
                        else lc->spec.selected=count-1;
                        lc->changed=1;changed=handled=1;
                    }
                    if(handled){if(lc->spec.selected<64)lc->list_selected_mask=1ULL<<lc->spec.selected;if(changed)host_ui_mark_dirty(ui);break;}
                }
                if(handled)continue;

                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    if(!ctl->used||ctl->window_handle!=g_ui_windows[ui].handle||!ctl->focused||
                       (ctl->spec.kind!=NYOTA_UI_CTRL_TAREA&&ctl->spec.kind!=NYOTA_UI_CTRL_TBOX&&ctl->spec.kind!=NYOTA_UI_CTRL_INPUT)||!ctl->spec.enabled)continue;
                    if(k==SDLK_LEFT){ctl->caret=host_ui_utf8_prev(ctl->spec.text,ctl->caret);handled=1;}
                    else if(k==SDLK_RIGHT){ctl->caret=host_ui_utf8_next(ctl->spec.text,ctl->caret);handled=1;}
                    else if((k==SDLK_UP||k==SDLK_DOWN)&&ctl->spec.kind==NYOTA_UI_CTRL_TAREA){
                        SDL_Rect tr;int x=0,row=0,lineh;TTF_Font *tf;
                        if(host_ui_control_rect_index(i,&tr)){
                            host_ui_tarea_caret_visual(ctl,tr,&x,&row);
                            tf=host_ui_get_font(ctl->spec.font,ctl->spec.font_size,ctl->spec.bold,ctl->spec.italic,ctl->spec.underline);
                            lineh=tf?TTF_FontLineSkip(tf):(int)ctl->spec.font_size+2;if(lineh<=0)lineh=(int)ctl->spec.font_size+2;
                            if(k==SDLK_UP&&row>0)row--;else if(k==SDLK_DOWN)row++;
                            ctl->caret=host_ui_tarea_caret_from_point(ctl,tr,tr.x+(int)ctl->spec.pad_x+x,
                                      tr.y+(int)ctl->spec.pad_y+row*lineh+lineh/2);
                        }
                        handled=1;
                    }
                    else if((e.key.keysym.mod & KMOD_CTRL) && k==SDLK_v && !ctl->spec.readonly && !e.key.repeat){
                        char *clip=SDL_GetClipboardText();
                        if(clip){changed=host_ui_tarea_insert(ctl,clip);SDL_free(clip);}
                        handled=1;
                    }
                    else if(k==SDLK_HOME){while(ctl->caret>0&&ctl->spec.text[ctl->caret-1]!='\n')ctl->caret=host_ui_utf8_prev(ctl->spec.text,ctl->caret);handled=1;}
                    else if(k==SDLK_END){size_t nn=strlen(ctl->spec.text);while(ctl->caret<nn&&ctl->spec.text[ctl->caret]!='\n')ctl->caret=host_ui_utf8_next(ctl->spec.text,ctl->caret);handled=1;}
                    else if(k==SDLK_BACKSPACE&&!e.key.repeat){changed=host_ui_tarea_backspace(ctl);handled=1;}
                    else if(k==SDLK_DELETE&&!e.key.repeat){changed=host_ui_tarea_delete(ctl);handled=1;}
                    else if((k==SDLK_RETURN||k==SDLK_KP_ENTER)&&!e.key.repeat){if(ctl->spec.kind==NYOTA_UI_CTRL_TAREA)changed=host_ui_tarea_insert(ctl,"\n");handled=1;}
                    if(handled)host_ui_mark_dirty(ui);
                    break;
                }
                if(handled)continue;
            }
            if (ui >= 0 && k == SDLK_TAB) {
                host_ui_focus_next(ui, (e.key.keysym.mod & KMOD_SHIFT) ? 1 : 0);
                host_ui_mark_dirty(ui);
                continue;
            }
            if (ui >= 0 && !e.key.repeat && (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE)) {
                int i, changed = 0;
                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    if(!ctl->used||ctl->window_handle!=g_ui_windows[ui].handle||!ctl->focused||
                       !ctl->spec.enabled||!host_ui_control_visible_index(i))continue;
                    ctl->pressed=1;
                    if(ctl->spec.kind==NYOTA_UI_CTRL_BUTTON||ctl->spec.kind==NYOTA_UI_CTRL_ICONBUTTON){
                        ctl->clicked=1;
                        if(ctl->spec.kind==NYOTA_UI_CTRL_ICONBUTTON&&ctl->spec.toggle){ctl->spec.checked=(uint8_t)!ctl->spec.checked;ctl->changed=1;}
                        changed=1;
                    }else if(ctl->spec.kind==NYOTA_UI_CTRL_SWITCH){
                        ctl->spec.checked=(uint8_t)!ctl->spec.checked;ctl->changed=1;changed=1;
                    }else if(ctl->spec.kind==NYOTA_UI_CTRL_CBOX){
                        ctl->spec.checked=(uint8_t)!ctl->spec.checked;changed=1;
                    }else if(ctl->spec.kind==NYOTA_UI_CTRL_RADIO){
                        int j;ctl->spec.checked=1;
                        if(ctl->spec.radio_group[0])
                            for(j=0;j<HOST_MAX_UI_CONTROLS;j++)
                                if(j!=i&&g_host_ui_controls[j].used&&g_host_ui_controls[j].spec.kind==NYOTA_UI_CTRL_RADIO&&
                                   g_host_ui_controls[j].window_handle==ctl->window_handle&&
                                   !strcmp(g_host_ui_controls[j].spec.radio_group,ctl->spec.radio_group))
                                    g_host_ui_controls[j].spec.checked=0;
                        changed=1;
                    }else if(ctl->spec.kind==NYOTA_UI_CTRL_COMBO){
                        ctl->combo_open=(uint8_t)!ctl->combo_open;ctl->combo_hover=-1;changed=1;
                    }else if(ctl->spec.kind==NYOTA_UI_CTRL_TAB){
                        int pi=host_ui_control_index_by_handle(ctl->parent_control_handle);
                        if(pi>=0){g_host_ui_controls[pi].spec.selected=ctl->spec.tab_index;changed=1;}
                    }
                    break;
                }
                if(changed)host_ui_mark_dirty(ui);
                continue;
            }
            if (k == SDLK_ESCAPE && g_win &&
                e.key.windowID == SDL_GetWindowID(g_win)) g_graph_closed = 1;
        }
        if (e.type == SDL_KEYUP) {
            SDL_Keycode k = e.key.keysym.sym;
            int ui = host_ui_index_by_window_id(e.key.windowID);
            if (k == SDLK_LSHIFT || k == SDLK_RSHIFT) g_shift = 0;
            if(ui>=0&&(k==SDLK_RETURN||k==SDLK_KP_ENTER||k==SDLK_SPACE)){
                int i,changed=0;
                for(i=0;i<HOST_MAX_UI_CONTROLS;i++){
                    HostUiControl *ctl=&g_host_ui_controls[i];
                    if(ctl->used&&ctl->window_handle==g_ui_windows[ui].handle&&ctl->pressed){
                        ctl->pressed=0;changed=1;
                    }
                }
                if(changed)host_ui_mark_dirty(ui);
            }
        }
    }
    if (!g_ui_initializing) host_ui_flush_dirty();
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
    if (TTF_Init() != 0)
        fprintf(stderr, "NyotaUI: SDL_ttf fallback: %s\n", TTF_GetError());
    (void)FcInit();
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
    uint32_t x, y;
    if (!u || !u->ren || u->w == 0 || u->h == 0) return -1;

    if (!u->background_cache) {
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
        u->background_cache = SDL_CreateTextureFromSurface(u->ren, surface);
        SDL_FreeSurface(surface);
        if (!u->background_cache) return -1;
        SDL_SetTextureBlendMode(u->background_cache, SDL_BLENDMODE_BLEND);
    }

    SDL_SetRenderDrawColor(u->ren, 0, 0, 0, 255);
    SDL_RenderClear(u->ren);
    SDL_RenderCopy(u->ren, u->background_cache, NULL, NULL);
    return 0;
}

static int host_ui_render_image(HostUiWindow *u) {
    SDL_Surface *surface;
    SDL_Texture *source = NULL, *saved = NULL;
    int sw, sh, dw, dh;
    SDL_Rect src, dst;
    if (!u || !u->ren || !u->background.image_path[0]) return -1;

    if (!u->background_cache) {
        surface = IMG_Load(u->background.image_path);
        if (!surface) return -1;
        sw = surface->w; sh = surface->h;
        source = SDL_CreateTextureFromSurface(u->ren, surface);
        SDL_FreeSurface(surface);
        if (!source || sw <= 0 || sh <= 0) { if (source) SDL_DestroyTexture(source); return -1; }

        u->background_cache = SDL_CreateTexture(u->ren, SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_TARGET, (int)u->w, (int)u->h);
        if (!u->background_cache) { SDL_DestroyTexture(source); return -1; }
        SDL_SetTextureBlendMode(u->background_cache, SDL_BLENDMODE_BLEND);

        saved = SDL_GetRenderTarget(u->ren);
        if (SDL_SetRenderTarget(u->ren, u->background_cache) != 0) {
            SDL_DestroyTexture(source);
            SDL_DestroyTexture(u->background_cache);
            u->background_cache = NULL;
            return -1;
        }
        SDL_SetRenderDrawColor(u->ren, 0, 0, 0, 255);
        SDL_RenderClear(u->ren);

        dst.x = 0; dst.y = 0; dst.w = (int)u->w; dst.h = (int)u->h;
        if (u->background.image_mode == NYOTA_UI_IMG_STRETCH) {
            SDL_RenderCopy(u->ren, source, NULL, &dst);
        } else if (u->background.image_mode == NYOTA_UI_IMG_NATIVE) {
            dst.w = sw; dst.h = sh;
            SDL_RenderCopy(u->ren, source, NULL, &dst);
        } else if (u->background.image_mode == NYOTA_UI_IMG_TILE) {
            int y, x;
            for (y = 0; y < (int)u->h; y += sh)
                for (x = 0; x < (int)u->w; x += sw) {
                    dst.x = x; dst.y = y; dst.w = sw; dst.h = sh;
                    SDL_RenderCopy(u->ren, source, NULL, &dst);
                }
        } else if (u->background.image_mode == NYOTA_UI_IMG_FIT) {
            if ((int64_t)sw * (int64_t)u->h > (int64_t)sh * (int64_t)u->w) {
                dw = (int)u->w; dh = (int)((int64_t)sh * u->w / sw);
            } else {
                dh = (int)u->h; dw = (int)((int64_t)sw * u->h / sh);
            }
            dst.w = dw; dst.h = dh;
            dst.x = ((int)u->w - dw) / 2; dst.y = ((int)u->h - dh) / 2;
            SDL_RenderCopy(u->ren, source, NULL, &dst);
        } else {
            src.x = 0; src.y = 0; src.w = sw; src.h = sh;
            if ((int64_t)sw * (int64_t)u->h > (int64_t)sh * (int64_t)u->w) {
                src.w = (int)((int64_t)sh * u->w / u->h); src.x = (sw - src.w) / 2;
            } else {
                src.h = (int)((int64_t)sw * u->h / u->w); src.y = (sh - src.h) / 2;
            }
            SDL_RenderCopy(u->ren, source, &src, &dst);
        }
        SDL_SetRenderTarget(u->ren, saved);
        SDL_DestroyTexture(source);
    }

    SDL_SetRenderDrawColor(u->ren, 0, 0, 0, 255);
    SDL_RenderClear(u->ren);
    SDL_RenderCopy(u->ren, u->background_cache, NULL, NULL);
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


static int host_ui_inside_rounded_corners(int x,int y,int w,int h,uint32_t radius,
                                          int tl,int tr,int br,int bl){
    int r=(int)radius, dx,dy;
    if(r<=0)return 1;
    if(r>w/2)r=w/2;
    if(r>h/2)r=h/2;
    if(x<r && y<r && tl){dx=r-1-x;dy=r-1-y;return dx*dx+dy*dy<=r*r;}
    if(x>=w-r && y<r && tr){dx=x-(w-r);dy=r-1-y;return dx*dx+dy*dy<=r*r;}
    if(x>=w-r && y>=h-r && br){dx=x-(w-r);dy=y-(h-r);return dx*dx+dy*dy<=r*r;}
    if(x<r && y>=h-r && bl){dx=r-1-x;dy=y-(h-r);return dx*dx+dy*dy<=r*r;}
    return 1;
}

static int host_ui_inside_rounded(int x,int y,int w,int h,uint32_t radius){
    return host_ui_inside_rounded_corners(x,y,w,h,radius,1,1,1,1);
}

static int host_ui_inside_rounded_corners_f(double x,double y,int w,int h,uint32_t radius,
                                            int tl,int tr,int br,int bl){
    double r=(double)radius,dx,dy;
    if(r<=0.0)return 1;
    if(r>(double)w/2.0)r=(double)w/2.0;
    if(r>(double)h/2.0)r=(double)h/2.0;
    if(x<r && y<r && tl){dx=r-x;dy=r-y;return dx*dx+dy*dy<=r*r;}
    if(x>(double)w-r && y<r && tr){dx=x-((double)w-r);dy=r-y;return dx*dx+dy*dy<=r*r;}
    if(x>(double)w-r && y>(double)h-r && br){dx=x-((double)w-r);dy=y-((double)h-r);return dx*dx+dy*dy<=r*r;}
    if(x<r && y>(double)h-r && bl){dx=r-x;dy=y-((double)h-r);return dx*dx+dy*dy<=r*r;}
    return 1;
}

static int host_ui_shape_sample_inside(const NyotaUiControlSpec *s,double x,double y){
    if(!s)return 0;
    if(s->kind==NYOTA_UI_CTRL_RADIO){
        double rr=(double)(s->w<s->h?s->w:s->h)/2.0;
        double dx=x-(double)s->w/2.0,dy=y-(double)s->h/2.0;
        return dx*dx+dy*dy<=rr*rr;
    }
    if(s->kind==NYOTA_UI_CTRL_DAREA && s->shape==NYOTA_UI_DAREA_CIRCLE){
        double rr=(double)(s->w<s->h?s->w:s->h)/2.0;
        double dx=x-(double)s->w/2.0,dy=y-(double)s->h/2.0;
        return dx*dx+dy*dy<=rr*rr;
    }
    if(s->kind==NYOTA_UI_CTRL_DAREA && s->shape==NYOTA_UI_DAREA_ELLIPSE){
        double rx=(double)s->w/2.0,ry=(double)s->h/2.0;
        double dx=(x-rx)/rx,dy=(y-ry)/ry;
        return dx*dx+dy*dy<=1.0;
    }
    if(s->radius && s->kind==NYOTA_UI_CTRL_TAB)
        return host_ui_inside_rounded_corners_f(x,y,(int)s->w,(int)s->h,s->radius,0,0,1,1);
    if(s->radius && s->kind==NYOTA_UI_CTRL_TABS)
        return host_ui_inside_rounded_corners_f(x,y,(int)s->w,(int)s->h,s->radius,1,1,0,0);
    if(s->radius)
        return host_ui_inside_rounded_corners_f(x,y,(int)s->w,(int)s->h,s->radius,1,1,1,1);
    return 1;
}

static uint8_t host_ui_shape_coverage(const NyotaUiControlSpec *s,int x,int y){
    static const double p[4]={0.125,0.375,0.625,0.875};
    int sx,sy,inside=0,r;
    if(!s)return 0;
    if(s->radius && s->kind!=NYOTA_UI_CTRL_RADIO &&
       !(s->kind==NYOTA_UI_CTRL_DAREA&&s->shape!=NYOTA_UI_DAREA_RECT)){
        r=(int)s->radius;if(r>(int)s->w/2)r=(int)s->w/2;if(r>(int)s->h/2)r=(int)s->h/2;
        if(s->kind==NYOTA_UI_CTRL_TAB){
            if(y<(int)s->h-r || (x>=r&&x<(int)s->w-r))return 16;
        }else if(s->kind==NYOTA_UI_CTRL_TABS){
            if(y>=r || (x>=r&&x<(int)s->w-r))return 16;
        }else{
            if((x>=r&&x<(int)s->w-r)||(y>=r&&y<(int)s->h-r))return 16;
        }
    }
    for(sy=0;sy<4;sy++)for(sx=0;sx<4;sx++)
        if(host_ui_shape_sample_inside(s,(double)x+p[sx],(double)y+p[sy]))inside++;
    return (uint8_t)inside;
}

typedef struct {
    SDL_Rect r;
    uint32_t radius;
    uint8_t tab;
} HostUiRoundedClip;

static int host_ui_collect_rounded_clips(int idx,HostUiRoundedClip *out,int cap){
    int p,n=0;
    if(idx<0||idx>=HOST_MAX_UI_CONTROLS||!g_host_ui_controls[idx].used)return 0;
    p=g_host_ui_controls[idx].parent_control_handle>0?
      host_ui_control_index_by_handle(g_host_ui_controls[idx].parent_control_handle):-1;
    while(p>=0){
        HostUiControl *pc=&g_host_ui_controls[p];
        SDL_Rect pr;
        if((pc->spec.kind==NYOTA_UI_CTRL_PANEL||pc->spec.kind==NYOTA_UI_CTRL_TAB||pc->spec.kind==NYOTA_UI_CTRL_STATBAR||pc->spec.kind==NYOTA_UI_CTRL_TOOLBAR||pc->spec.kind==NYOTA_UI_CTRL_FRAME)&&pc->spec.clip&&
           pc->spec.radius&&host_ui_control_rect_index(p,&pr)&&n<cap){
            out[n].r=pr;out[n].radius=pc->spec.radius;out[n].tab=(uint8_t)(pc->spec.kind==NYOTA_UI_CTRL_TAB);n++;
        }
        p=pc->parent_control_handle>0?host_ui_control_index_by_handle(pc->parent_control_handle):-1;
    }
    return n;
}

static int host_ui_point_in_clipset(const HostUiRoundedClip *clips,int n,double gx,double gy){
    int i;
    for(i=0;i<n;i++){
        const HostUiRoundedClip *c=&clips[i];
        double lx=gx-(double)c->r.x,ly=gy-(double)c->r.y;
        if(lx<0.0||ly<0.0||lx>=(double)c->r.w||ly>=(double)c->r.h)return 0;
        if(c->tab){
            if(!host_ui_inside_rounded_corners_f(lx,ly,c->r.w,c->r.h,c->radius,0,0,1,1))return 0;
        }else if(!host_ui_inside_rounded_corners_f(lx,ly,c->r.w,c->r.h,c->radius,1,1,1,1))return 0;
    }
    return 1;
}

static int host_ui_point_in_rounded_ancestor(int idx,double gx,double gy){
    HostUiRoundedClip clips[16];int n=host_ui_collect_rounded_clips(idx,clips,16);
    return host_ui_point_in_clipset(clips,n,gx,gy);
}

static void host_ui_apply_ancestor_mask(int idx,SDL_Surface *sf,int gx,int gy){
    static const double p[4]={0.125,0.375,0.625,0.875};
    HostUiRoundedClip clips[16];
    int n,x,y,sx,sy,i,needs_mask=0;
    if(idx<0||!sf)return;
    n=host_ui_collect_rounded_clips(idx,clips,16);
    if(n<=0)return;

    /* Wiekszosc dzieci panelu nie dotyka jego zaokraglonych rogow.
       W takim przypadku prostokatny SDL clip juz wystarcza i kosztowna
       maska 4x4 AA dla kazdego piksela jest zbedna. */
    for(i=0;i<n;i++){
        const HostUiRoundedClip *c=&clips[i];
        int rr=(int)c->radius;
        int x1=gx, y1=gy, x2=gx+sf->w, y2=gy+sf->h;
        int left = x1 < c->r.x + rr;
        int right = x2 > c->r.x + c->r.w - rr;
        int top = y1 < c->r.y + rr;
        int bottom = y2 > c->r.y + c->r.h - rr;
        if(c->tab) {
            if((left||right) && bottom) { needs_mask=1; break; }
        } else {
            if((left||right) && (top||bottom)) { needs_mask=1; break; }
        }
    }
    if(!needs_mask)return;
    if(SDL_LockSurface(sf)!=0)return;
    for(y=0;y<sf->h;y++){
        uint32_t *row=(uint32_t *)((uint8_t *)sf->pixels+y*sf->pitch);
        for(x=0;x<sf->w;x++){
            int cov=0;
            if(host_ui_point_in_clipset(clips,n,(double)(gx+x)+0.5,(double)(gy+y)+0.5)){
                int edge=0,i;
                for(i=0;i<n;i++){
                    const HostUiRoundedClip *c=&clips[i];
                    int lx=gx+x-c->r.x,ly=gy+y-c->r.y,rr=(int)c->radius;
                    if((lx<rr||lx>=c->r.w-rr) &&
                       (c->tab ? ly>=c->r.h-rr : (ly<rr||ly>=c->r.h-rr))){edge=1;break;}
                }
                if(!edge)cov=16;
            }
            if(cov!=16){
                if(cov==0){
                    for(sy=0;sy<4;sy++)for(sx=0;sx<4;sx++)
                        if(host_ui_point_in_clipset(clips,n,(double)(gx+x)+p[sx],(double)(gy+y)+p[sy]))cov++;
                }
                if(cov<16){
                    uint8_t r,g,b,a;
                    SDL_GetRGBA(row[x],sf->format,&r,&g,&b,&a);
                    a=(uint8_t)(((uint32_t)a*(uint32_t)cov+8u)/16u);
                    row[x]=SDL_MapRGBA(sf->format,r,g,b,a);
                }
            }
        }
    }
    SDL_UnlockSurface(sf);
}

static int host_ui_has_rounded_ancestor(int idx){
    int p;
    if(idx<0||idx>=HOST_MAX_UI_CONTROLS||!g_host_ui_controls[idx].used)return 0;
    p=g_host_ui_controls[idx].parent_control_handle>0?
      host_ui_control_index_by_handle(g_host_ui_controls[idx].parent_control_handle):-1;
    while(p>=0){
        HostUiControl *pc=&g_host_ui_controls[p];
        if((pc->spec.kind==NYOTA_UI_CTRL_PANEL||pc->spec.kind==NYOTA_UI_CTRL_TAB||pc->spec.kind==NYOTA_UI_CTRL_STATBAR||pc->spec.kind==NYOTA_UI_CTRL_TOOLBAR||pc->spec.kind==NYOTA_UI_CTRL_FRAME)&&pc->spec.clip&&pc->spec.radius)return 1;
        p=pc->parent_control_handle>0?host_ui_control_index_by_handle(pc->parent_control_handle):-1;
    }
    return 0;
}

static void host_ui_draw_line_masked(SDL_Renderer *ren,int idx,int x0,int y0,int x1,int y1){
    int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
    if(!ren)return;
    if(!host_ui_has_rounded_ancestor(idx)){SDL_RenderDrawLine(ren,x0,y0,x1,y1);return;}
    for(;;){
        if(host_ui_point_in_rounded_ancestor(idx,(double)x0+0.5,(double)y0+0.5))SDL_RenderDrawPoint(ren,x0,y0);
        if(x0==x1&&y0==y1)break;
        {int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}}
    }
}

static void host_ui_fill_rect_masked(SDL_Renderer *ren,int idx,SDL_Rect q){
    int y,x,run=-1;
    if(!ren||q.w<=0||q.h<=0)return;
    if(!host_ui_has_rounded_ancestor(idx)){SDL_RenderFillRect(ren,&q);return;}
    for(y=q.y;y<q.y+q.h;y++){
        run=-1;
        for(x=q.x;x<q.x+q.w;x++){
            int ok=host_ui_point_in_rounded_ancestor(idx,(double)x+0.5,(double)y+0.5);
            if(ok&&run<0)run=x;
            if((!ok||x==q.x+q.w-1)&&run>=0){
                int end=(ok&&x==q.x+q.w-1)?x:x-1;
                SDL_RenderDrawLine(ren,run,y,end,y);run=-1;
            }
        }
    }
}

static int host_ui_shape_sample_inside_inset(const NyotaUiControlSpec *s,double x,double y,uint32_t inset){
    NyotaUiControlSpec inner;
    if(!s)return 0;
    if(inset==0)return host_ui_shape_sample_inside(s,x,y);
    if(s->w<=2u*inset||s->h<=2u*inset)return 0;
    if(x<(double)inset||y<(double)inset||
       x>(double)s->w-(double)inset||y>(double)s->h-(double)inset)return 0;
    inner=*s;
    inner.w=s->w-2u*inset;
    inner.h=s->h-2u*inset;
    inner.radius=s->radius>inset?s->radius-inset:0;
    return host_ui_shape_sample_inside(&inner,x-(double)inset,y-(double)inset);
}

static uint8_t host_ui_border_coverage(const NyotaUiControlSpec *s,int x,int y,uint32_t width){
    static const double p[4]={0.125,0.375,0.625,0.875};
    int sx,sy,outer=0,inner=0;
    for(sy=0;sy<4;sy++)for(sx=0;sx<4;sx++){
        double xx=(double)x+p[sx],yy=(double)y+p[sy];
        if(host_ui_shape_sample_inside(s,xx,yy))outer++;
        if(host_ui_shape_sample_inside_inset(s,xx,yy,width))inner++;
    }
    return (uint8_t)(outer>inner?outer-inner:0);
}

static SDL_Surface *host_ui_border_surface(const NyotaUiControlSpec *s,NyotaColor color,uint32_t width){
    SDL_Surface *sf;
    uint32_t x,y;
    if(!s||!s->w||!s->h||!width||color.mode==NYOTA_COLOR_TRANSPARENT)return NULL;
    sf=SDL_CreateRGBSurfaceWithFormat(0,(int)s->w,(int)s->h,32,SDL_PIXELFORMAT_RGBA32);
    if(!sf)return NULL;
    SDL_FillRect(sf,NULL,SDL_MapRGBA(sf->format,0,0,0,0));
    if(SDL_LockSurface(sf)!=0){SDL_FreeSurface(sf);return NULL;}
    for(y=0;y<s->h;y++){
        uint32_t *row=(uint32_t *)((uint8_t *)sf->pixels+y*sf->pitch);
        for(x=0;x<s->w;x++){
            if(s->radius && s->kind!=NYOTA_UI_CTRL_RADIO &&
               !(s->kind==NYOTA_UI_CTRL_DAREA&&s->shape!=NYOTA_UI_DAREA_RECT)) {
                uint32_t r=s->radius;
                int near_corner;
                if(r>s->w/2)r=s->w/2;
                if(r>s->h/2)r=s->h/2;
                near_corner=((x<r||x>=s->w-r) &&
                             ((s->kind==NYOTA_UI_CTRL_TAB && y>=s->h-r) ||
                              (s->kind==NYOTA_UI_CTRL_TABS && y<r) ||
                              (s->kind!=NYOTA_UI_CTRL_TAB && s->kind!=NYOTA_UI_CTRL_TABS &&
                               (y<r||y>=s->h-r))));
                if(!near_corner && x>=width && x<s->w-width &&
                   y>=width && y<s->h-width) continue;
            }
            {
                uint8_t cov=host_ui_border_coverage(s,(int)x,(int)y,width);
                uint8_t a=(uint8_t)(((uint32_t)color.a*(uint32_t)cov+8u)/16u);
                if(a)row[x]=SDL_MapRGBA(sf->format,color.r,color.g,color.b,a);
            }
        }
    }
    SDL_UnlockSurface(sf);
    return sf;
}
static SDL_Surface *host_ui_control_background_surface(const NyotaUiControlSpec *s,
                                                        const NyotaUiBackground *bg) {
    SDL_Surface *dst = NULL, *src = NULL, *conv = NULL;
    uint32_t x, y;
    if (!s || !bg || !s->w || !s->h) return NULL;

    /* Calkowicie przezroczyste tlo niczego nie rysuje. Nie tworz Surface/Texture
       tylko po to, aby skopiowac miliony pikseli z alfa 0. */
    if (bg->kind == NYOTA_UI_BG_COLOR &&
        bg->colors[0].mode == NYOTA_COLOR_TRANSPARENT) return NULL;

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
    } else if (bg->kind == NYOTA_UI_BG_COLOR) {
        NyotaColor cc = bg->colors[0];
        uint8_t a = cc.mode == NYOTA_COLOR_TRANSPARENT ? 0 : cc.a;
        SDL_FillRect(dst, NULL, SDL_MapRGBA(dst->format, cc.r, cc.g, cc.b, a));
    } else {
        if (SDL_LockSurface(dst) != 0) { SDL_FreeSurface(dst); return NULL; }
        for (y = 0; y < s->h; y++) {
            uint32_t *row = (uint32_t *)((uint8_t *)dst->pixels + y * dst->pitch);
            for (x = 0; x < s->w; x++) {
                uint8_t r=0,g=0,b=0,a=0;
                double t = host_ui_gradient_t(bg, x, y, s->w, s->h);
                host_ui_lerp_color(bg, t, &r, &g, &b, &a);
                row[x] = SDL_MapRGBA(dst->format, r, g, b, a);
            }
        }
        SDL_UnlockSurface(dst);
    }

    if ((s->kind == NYOTA_UI_CTRL_DAREA && s->shape != NYOTA_UI_DAREA_RECT) || s->kind == NYOTA_UI_CTRL_RADIO || s->radius) {
        if (SDL_LockSurface(dst) == 0) {
            if(s->radius && s->kind != NYOTA_UI_CTRL_RADIO &&
               !(s->kind == NYOTA_UI_CTRL_DAREA && s->shape != NYOTA_UI_DAREA_RECT)) {
                uint32_t r=s->radius;
                uint32_t yy,xx;
                if(r>s->w/2)r=s->w/2;
                if(r>s->h/2)r=s->h/2;
                for(yy=0;yy<r;yy++) {
                    uint32_t ys[2]={yy,s->h-1-yy};
                    int ycount=(s->kind==NYOTA_UI_CTRL_TAB||s->kind==NYOTA_UI_CTRL_TABS)?1:2;
                    int yi;
                    if(s->kind==NYOTA_UI_CTRL_TAB) ys[0]=s->h-1-yy;
                    else if(s->kind==NYOTA_UI_CTRL_TABS) ys[0]=yy;
                    for(yi=0;yi<ycount;yi++) {
                        uint32_t py=ys[yi];
                        uint32_t *row=(uint32_t *)((uint8_t *)dst->pixels+py*dst->pitch);
                        for(xx=0;xx<r;xx++) {
                            uint32_t xs[2]={xx,s->w-1-xx};
                            int xi;
                            for(xi=0;xi<2;xi++) {
                                uint32_t px=xs[xi];
                                uint8_t cov=host_ui_shape_coverage(s,(int)px,(int)py);
                                if(cov<16){
                                    uint8_t rr,gg,bb,aa;
                                    SDL_GetRGBA(row[px],dst->format,&rr,&gg,&bb,&aa);
                                    aa=(uint8_t)(((uint32_t)aa*(uint32_t)cov+8u)/16u);
                                    row[px]=SDL_MapRGBA(dst->format,rr,gg,bb,aa);
                                }
                            }
                        }
                    }
                }
            } else {
                for (y = 0; y < s->h; y++) {
                    uint32_t *row = (uint32_t *)((uint8_t *)dst->pixels + y * dst->pitch);
                    for (x = 0; x < s->w; x++) {
                        uint8_t cov=host_ui_shape_coverage(s,(int)x,(int)y);
                        if(cov<16){
                            uint8_t rr,gg,bb,aa;
                            SDL_GetRGBA(row[x],dst->format,&rr,&gg,&bb,&aa);
                            aa=(uint8_t)(((uint32_t)aa*(uint32_t)cov+8u)/16u);
                            row[x]=SDL_MapRGBA(dst->format,rr,gg,bb,aa);
                        }
                    }
                }
            }
            SDL_UnlockSurface(dst);
        }
    }
    return dst;
}

static void host_ui_draw_text_fallback(SDL_Renderer *ren, const NyotaUiControlSpec *s, SDL_Rect r) {
    const char *text;
    int scale, charw, charh, len, tx, ty, i, row, col;
    NyotaColor color;
    if (!ren || !s || !(text = s->text) || !text[0] || s->kind == NYOTA_UI_CTRL_PANEL) return;
    color = s->text_color;
    if (color.mode == NYOTA_COLOR_TRANSPARENT) return;
    scale = (int)((s->font_size + 7u) / 8u);
    if (scale < 1) scale = 1;
    if (scale > 16) scale = 16;
    charw = 8 * scale; charh = 8 * scale; len = (int)strlen(text);
    tx = r.x + (int)s->pad_x;
    if (s->halign == NYOTA_UI_ALIGN_CENTER) tx = r.x + (r.w - len * charw) / 2;
    else if (s->halign == NYOTA_UI_ALIGN_RIGHT) tx = r.x + r.w - len * charw - (int)s->pad_x;
    ty = r.y + (int)s->pad_y;
    if (s->valign == NYOTA_UI_VALIGN_MIDDLE) ty = r.y + (r.h - charh) / 2;
    else if (s->valign == NYOTA_UI_VALIGN_BOTTOM) ty = r.y + r.h - charh - (int)s->pad_y;
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
    if (s->underline)
        SDL_RenderDrawLine(ren, tx, ty + charh - 1, tx + len * charw - 1, ty + charh - 1);
}

static void host_ui_draw_text(SDL_Renderer *ren, const NyotaUiControlSpec *s, SDL_Rect r) {
    TTF_Font *font;
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Color color;
    SDL_Rect dst;
    int maxw;
    if (!ren || !s || !s->text[0] || s->kind == NYOTA_UI_CTRL_PANEL) return;
    if (s->text_color.mode == NYOTA_COLOR_TRANSPARENT) return;
    font = host_ui_get_font(s->font, s->font_size, s->bold, s->italic, s->underline);
    if (!font) {
        host_ui_draw_text_fallback(ren, s, r);
        return;
    }
    color.r=s->text_color.r; color.g=s->text_color.g; color.b=s->text_color.b; color.a=s->text_color.a;
    maxw = r.w > (int)(2u*s->pad_x) ? r.w - (int)(2u*s->pad_x) : r.w;
    if (s->wrap && maxw > 0)
        surface = TTF_RenderUTF8_Blended_Wrapped(font, s->text, color, (Uint32)maxw);
    else
        surface = TTF_RenderUTF8_Blended(font, s->text, color);
    if (!surface) {
        host_ui_draw_text_fallback(ren, s, r);
        return;
    }
    dst.w=surface->w; dst.h=surface->h;
    dst.x=r.x+(int)s->pad_x;
    if(s->halign==NYOTA_UI_ALIGN_CENTER) dst.x=r.x+(r.w-dst.w)/2;
    else if(s->halign==NYOTA_UI_ALIGN_RIGHT) dst.x=r.x+r.w-dst.w-(int)s->pad_x;
    dst.y=r.y+(int)s->pad_y;
    if(s->valign==NYOTA_UI_VALIGN_MIDDLE) dst.y=r.y+(r.h-dst.h)/2;
    else if(s->valign==NYOTA_UI_VALIGN_BOTTOM) dst.y=r.y+r.h-dst.h-(int)s->pad_y;
    if(g_ui_draw_clip_idx>=0)host_ui_apply_ancestor_mask(g_ui_draw_clip_idx,surface,dst.x,dst.y);
    texture = SDL_CreateTextureFromSurface(ren, surface);
    if (!texture) { SDL_FreeSurface(surface); host_ui_draw_text_fallback(ren,s,r); return; }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(ren,texture,NULL,&dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
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

static void host_ui_draw_bottom_rounded_rect(SDL_Renderer *ren, SDL_Rect q, int radius) {
    const double pi = 3.14159265358979323846;
    int seg, nseg=12, px,py,nx,ny;
    if(!ren||q.w<=1||q.h<=1)return;
    if(radius<=0){SDL_RenderDrawRect(ren,&q);return;}
    if(radius>q.w/2)radius=q.w/2;
    if(radius>q.h/2)radius=q.h/2;
    SDL_RenderDrawLine(ren,q.x,q.y,q.x+q.w-1,q.y);
    SDL_RenderDrawLine(ren,q.x,q.y,q.x,q.y+q.h-1-radius);
    SDL_RenderDrawLine(ren,q.x+q.w-1,q.y,q.x+q.w-1,q.y+q.h-1-radius);
    px=q.x;py=q.y+q.h-1-radius;
    for(seg=0;seg<=nseg;seg++){
        double a=pi-(pi/2.0)*(double)seg/nseg;
        nx=(int)lrint(q.x+radius+cos(a)*radius);
        ny=(int)lrint(q.y+q.h-1-radius+sin(a)*radius);
        if(seg)SDL_RenderDrawLine(ren,px,py,nx,ny);px=nx;py=ny;
    }
    SDL_RenderDrawLine(ren,q.x+radius,q.y+q.h-1,q.x+q.w-1-radius,q.y+q.h-1);
    px=q.x+q.w-1-radius;py=q.y+q.h-1;
    for(seg=0;seg<=nseg;seg++){
        double a=(pi/2.0)-(pi/2.0)*(double)seg/nseg;
        nx=(int)lrint(q.x+q.w-1-radius+cos(a)*radius);
        ny=(int)lrint(q.y+q.h-1-radius+sin(a)*radius);
        if(seg)SDL_RenderDrawLine(ren,px,py,nx,ny);px=nx;py=ny;
    }
}

static void host_ui_draw_border(SDL_Renderer *ren, const HostUiControl *ctl, SDL_Rect r) {
    uint32_t k, width;
    NyotaColor color;
    if (!ren || !ctl || !ctl->spec.border) return;
    color = (ctl->spec.kind == NYOTA_UI_CTRL_DAREA && ctl->hover && ctl->spec.enabled) ?
            ctl->spec.border_color_over : ctl->spec.border_color;
    width = (ctl->spec.kind == NYOTA_UI_CTRL_DAREA && ctl->hover && ctl->spec.enabled) ?
            ctl->spec.border_width_over : ctl->spec.border_width;
    if(!ctl->spec.enabled) color = host_ui_tint_color(color,-36);
    if (color.mode == NYOTA_COLOR_TRANSPARENT || !width) return;
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    if ((ctl->spec.kind == NYOTA_UI_CTRL_DAREA && ctl->spec.shape != NYOTA_UI_DAREA_RECT) ||
        ctl->spec.kind == NYOTA_UI_CTRL_RADIO || ctl->spec.radius) {
        SDL_Surface *sf=host_ui_border_surface(&ctl->spec,color,width);
        if(sf){
            int idx=(int)(ctl-g_host_ui_controls);
            host_ui_apply_ancestor_mask(idx,sf,r.x,r.y);
            SDL_Texture *tx=SDL_CreateTextureFromSurface(ren,sf);
            SDL_FreeSurface(sf);
            if(tx){
                SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);
                SDL_RenderCopy(ren,tx,NULL,&r);
                SDL_DestroyTexture(tx);
            }
        }
        return;
    }

    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
    for (k = 0; k < width; k++) {
        SDL_Rect q = {r.x + (int)k, r.y + (int)k, r.w - (int)(2*k), r.h - (int)(2*k)};
        if (q.w <= 1 || q.h <= 1) break;
        SDL_RenderDrawRect(ren, &q);
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
        if((pc->spec.kind==NYOTA_UI_CTRL_PANEL||pc->spec.kind==NYOTA_UI_CTRL_TAB||pc->spec.kind==NYOTA_UI_CTRL_STATBAR||pc->spec.kind==NYOTA_UI_CTRL_TOOLBAR||pc->spec.kind==NYOTA_UI_CTRL_FRAME) && pc->spec.clip && host_ui_control_rect_index(p,&pr)){
            if(!SDL_IntersectRect(clip,&pr,&inter)){clip->w=clip->h=0;return 1;} *clip=inter;
        }
        p=pc->parent_control_handle>0?host_ui_control_index_by_handle(pc->parent_control_handle):-1;
    }
    return 1;
}

static void host_ui_draw_simple_text(SDL_Renderer *ren,const char *text,NyotaColor c,SDL_Rect r,uint32_t fs){
    NyotaUiControlSpec t; memset(&t,0,sizeof(t)); t.font_size=fs;t.text_color=c;t.halign=NYOTA_UI_ALIGN_LEFT;t.valign=NYOTA_UI_VALIGN_MIDDLE;t.pad_x=8;t.pad_y=4;strncpy(t.font,"SYSTEM",sizeof(t.font)-1);strncpy(t.text,text,sizeof(t.text)-1);host_ui_draw_text(ren,&t,r);
}
static int host_ui_item_at(const char *items,uint32_t wanted,char *out,size_t cap){
    uint32_t cur=0,start=0,i=0;if(!out||!cap)return 0;out[0]='\0';
    while(1){char ch=items[i];if(ch=='\n'||ch=='\0'){if(cur==wanted){size_t n=i-start;if(n>=cap)n=cap-1;memcpy(out,items+start,n);out[n]='\0';return 1;}cur++;start=i+1;if(ch=='\0')break;}i++;}return 0;
}
static void host_ui_draw_shadow(SDL_Renderer *ren,const HostUiControl *ctl,SDL_Rect r){
    const NyotaUiControlSpec *s=ctl?&ctl->spec:NULL;
    int dx=0,dy=0,d=s?(int)s->shadow_depth:0;SDL_Rect q=r;
    if(!ren||!ctl||!s||s->shadow==NYOTA_UI_SHADOW_OFF||!d)return;
    if(s->shadow==NYOTA_UI_SHADOW_R||s->shadow==NYOTA_UI_SHADOW_RU||s->shadow==NYOTA_UI_SHADOW_RD)dx=d;
    if(s->shadow==NYOTA_UI_SHADOW_L||s->shadow==NYOTA_UI_SHADOW_LU||s->shadow==NYOTA_UI_SHADOW_LD)dx=-d;
    if(s->shadow==NYOTA_UI_SHADOW_U||s->shadow==NYOTA_UI_SHADOW_RU||s->shadow==NYOTA_UI_SHADOW_LU)dy=-d;
    if(s->shadow==NYOTA_UI_SHADOW_D||s->shadow==NYOTA_UI_SHADOW_RD||s->shadow==NYOTA_UI_SHADOW_LD)dy=d;
    q.x+=dx;q.y+=dy;
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    if(s->kind==NYOTA_UI_CTRL_SEP){
        SDL_SetRenderDrawColor(ren,s->shadow_color.r,s->shadow_color.g,s->shadow_color.b,s->shadow_color.a);
        if(s->orientation==NYOTA_UI_SEP_VERTICAL)q.w=(int)s->sep_thickness;else q.h=(int)s->sep_thickness;
        SDL_RenderFillRect(ren,&q);
    }else{
        NyotaUiControlSpec ss=*s;
        NyotaUiBackground bg;
        SDL_Surface *sf;
        SDL_Texture *tx;
        memset(&bg,0,sizeof(bg));
        bg.kind=NYOTA_UI_BG_COLOR;bg.color_count=1;bg.colors[0]=s->shadow_color;
        ss.w=(uint32_t)r.w;ss.h=(uint32_t)r.h;
        sf=host_ui_control_background_surface(&ss,&bg);
        if(sf){
            {
                int idx=(int)(ctl-g_host_ui_controls);
                if(idx>=0&&idx<HOST_MAX_UI_CONTROLS)host_ui_apply_ancestor_mask(idx,sf,q.x,q.y);
            }
            tx=SDL_CreateTextureFromSurface(ren,sf);
            SDL_FreeSurface(sf);
            if(tx){
                SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);
                SDL_RenderCopy(ren,tx,NULL,&q);
                SDL_DestroyTexture(tx);
            }
        }
    }
}
static uint8_t host_ui_shift_chan(uint8_t v,int delta){
    int n=(int)v+delta;
    if(n<0)n=0;if(n>255)n=255;return (uint8_t)n;
}
static void host_ui_tint_background(const NyotaUiBackground *src,NyotaUiBackground *dst,int delta){
    uint32_t i;*dst=*src;
    if(dst->kind==NYOTA_UI_BG_IMAGE)return;
    for(i=0;i<dst->color_count && i<NYOTA_UI_MAX_GRAD_COLORS;i++){
        if(dst->colors[i].mode==NYOTA_COLOR_SOLID){
            dst->colors[i].r=host_ui_shift_chan(dst->colors[i].r,delta);
            dst->colors[i].g=host_ui_shift_chan(dst->colors[i].g,delta);
            dst->colors[i].b=host_ui_shift_chan(dst->colors[i].b,delta);
        }
    }
}
static NyotaColor host_ui_tint_color(NyotaColor c,int delta){
    if(c.mode==NYOTA_COLOR_SOLID){
        c.r=host_ui_shift_chan(c.r,delta);c.g=host_ui_shift_chan(c.g,delta);c.b=host_ui_shift_chan(c.b,delta);
    }
    return c;
}

static void host_ui_draw_tab_header(int idx,SDL_Renderer *ren){
    HostUiControl *c=&g_host_ui_controls[idx];
    SDL_Rect r;SDL_Surface *sf;SDL_Texture *tx;NyotaUiControlSpec tmp;
    NyotaUiBackground bg;
    NyotaColor bc,tc;
    int active;
    if(!host_ui_tab_header_rect(idx,&r))return;
    active=host_ui_tab_is_active(idx);
    host_ui_tint_background(&c->spec.tab_background,&bg,!c->spec.enabled?-28:(active?8:(c->hover?3:-8)));
    if(c->spec.enabled&&c->pressed)host_ui_tint_background(&c->spec.tab_background,&bg,-14);

    memset(&tmp,0,sizeof(tmp));
    tmp.kind=NYOTA_UI_CTRL_TABS;
    tmp.w=(uint32_t)r.w;tmp.h=(uint32_t)r.h;tmp.background=bg;tmp.radius=c->spec.tab_radius;
    sf=host_ui_control_background_surface(&tmp,&tmp.background);
    if(sf){tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);if(tx){SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);SDL_RenderCopy(ren,tx,NULL,&r);SDL_DestroyTexture(tx);}}

    bc=host_ui_tint_color(c->spec.tab_border_color,!c->spec.enabled?-32:(active?18:(c->hover?8:0)));
    if(c->spec.tab_border&&bc.mode!=NYOTA_COLOR_TRANSPARENT){
        int k;SDL_SetRenderDrawColor(ren,bc.r,bc.g,bc.b,bc.a);
        for(k=0;k<(int)c->spec.tab_border_width;k++){
            SDL_RenderDrawLine(ren,r.x+k+(int)c->spec.tab_radius,r.y+k,r.x+r.w-1-k-(int)c->spec.tab_radius,r.y+k);
            SDL_RenderDrawLine(ren,r.x+k,r.y+(int)c->spec.tab_radius,r.x+k,r.y+r.h-1);
            SDL_RenderDrawLine(ren,r.x+r.w-1-k,r.y+(int)c->spec.tab_radius,r.x+r.w-1-k,r.y+r.h-1);
        }
    }

    memset(&tmp,0,sizeof(tmp));
    strncpy(tmp.font,c->spec.tab_font,sizeof(tmp.font)-1);
    tmp.font_size=c->spec.tab_font_size;
    tmp.pad_x=c->spec.tab_pad_x;
    tmp.pad_y=c->spec.tab_pad_y;
    tc=host_ui_tint_color(c->spec.tab_text_color,!c->spec.enabled?-62:(active?16:0));
    tmp.text_color=tc;tmp.bold=c->spec.tab_bold;tmp.italic=c->spec.tab_italic;tmp.underline=c->spec.tab_underline;
    tmp.halign=NYOTA_UI_ALIGN_CENTER;tmp.valign=NYOTA_UI_VALIGN_MIDDLE;
    strncpy(tmp.text,c->spec.text,sizeof(tmp.text)-1);
    host_ui_draw_text(ren,&tmp,r);
    if(c->focused&&c->spec.enabled){
        NyotaColor fc=host_ui_tint_color(c->spec.tab_border_color,78);
        int x1=r.x+(int)c->spec.tab_radius+4,x2=r.x+r.w-1-(int)c->spec.tab_radius-4;
        if(fc.mode==NYOTA_COLOR_TRANSPARENT){fc.r=110;fc.g=165;fc.b=235;fc.a=255;}
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren,fc.r,fc.g,fc.b,220);
        if(x2>x1){SDL_RenderDrawLine(ren,x1,r.y+1,x2,r.y+1);SDL_RenderDrawLine(ren,x1,r.y+2,x2,r.y+2);}
    }
}

static void host_ui_draw_focus_ring(SDL_Renderer *ren,const HostUiControl *ctl,SDL_Rect r){
    SDL_Rect q=r;
    NyotaColor c;
    if(!ren||!ctl||!ctl->focused||!ctl->spec.enabled)return;
    if(ctl->spec.kind==NYOTA_UI_CTRL_TAB)return;
    c=host_ui_tint_color(ctl->spec.border_color,70);
    if(c.mode==NYOTA_COLOR_TRANSPARENT){c.r=110;c.g=165;c.b=235;c.a=255;c.mode=NYOTA_COLOR_SOLID;}
    c.a=190;
    q.x-=2;q.y-=2;q.w+=4;q.h+=4;
    if(ctl->spec.kind==NYOTA_UI_CTRL_RADIO||ctl->spec.radius||
       (ctl->spec.kind==NYOTA_UI_CTRL_DAREA&&ctl->spec.shape!=NYOTA_UI_DAREA_RECT)){
        NyotaUiControlSpec fs=ctl->spec;
        SDL_Surface *sf;
        SDL_Texture *tx;
        fs.w=(uint32_t)q.w;fs.h=(uint32_t)q.h;
        if(fs.radius)fs.radius+=2;
        sf=host_ui_border_surface(&fs,c,1);
        if(sf){
            int idx=(int)(ctl-g_host_ui_controls);
            host_ui_apply_ancestor_mask(idx,sf,q.x,q.y);
            tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);
            if(tx){SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);SDL_RenderCopy(ren,tx,NULL,&q);SDL_DestroyTexture(tx);}
        }
    }else{
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
        SDL_RenderDrawRect(ren,&q);
    }
}

static int host_ui_tarea_next_line(TTF_Font *font,const char *text,uint32_t start,uint32_t maxw,
                                  uint32_t *end,uint32_t *next){
    uint32_t n=(uint32_t)strlen(text),p=start,last_good=start;
    char buf[NYOTA_UI_TEXT_MAX];
    int w=0,h=0;
    if(start>=n){*end=start;*next=start;return 0;}
    p=start;
    while(p<n){
        uint32_t q=host_ui_utf8_next(text,p);
        size_t len;
        if(text[p]=='\n'){*end=p;*next=q;return 1;}
        len=q-start;if(len>=sizeof(buf))len=sizeof(buf)-1;
        memcpy(buf,text+start,len);buf[len]='\0';
        if(maxw&&TTF_SizeUTF8(font,buf,&w,&h)==0&&w>(int)maxw&&last_good>start){
            *end=last_good;*next=last_good;return 1;
        }
        last_good=q;p=q;
    }
    *end=n;*next=n;return 1;
}

static uint32_t host_ui_tarea_caret_from_point(HostUiControl *ctl,SDL_Rect r,int mx,int my){
    TTF_Font *font;
    uint32_t start=0,end=0,next=0,row=0,target_row,maxw;
    int lineh,targetx;
    if(!ctl)return 0;
    font=host_ui_get_font(ctl->spec.font,ctl->spec.font_size,ctl->spec.bold,ctl->spec.italic,ctl->spec.underline);
    if(!font)return (uint32_t)strlen(ctl->spec.text);
    lineh=TTF_FontLineSkip(font);if(lineh<=0)lineh=(int)ctl->spec.font_size+2;
    target_row=my<=r.y+(int)ctl->spec.pad_y?0u:(uint32_t)((my-r.y-(int)ctl->spec.pad_y)/lineh);
    targetx=mx-r.x-(int)ctl->spec.pad_x;if(targetx<0)targetx=0;
    maxw=r.w>(int)(2u*ctl->spec.pad_x)?(uint32_t)(r.w-(int)(2u*ctl->spec.pad_x)):0;
    while(start<(uint32_t)strlen(ctl->spec.text)){
        if(ctl->spec.wrap)host_ui_tarea_next_line(font,ctl->spec.text,start,maxw,&end,&next);
        else{
            const char *nl=strchr(ctl->spec.text+start,'\n');
            end=nl?(uint32_t)(nl-ctl->spec.text):(uint32_t)strlen(ctl->spec.text);
            next=nl?end+1:end;
        }
        if(row==target_row){
            uint32_t p=start,prev=start;int pw=0,w=0,h=0;
            while(p<end){
                uint32_t q=host_ui_utf8_next(ctl->spec.text,p);
                char buf[NYOTA_UI_TEXT_MAX];size_t len=q-start;
                if(len>=sizeof(buf))len=sizeof(buf)-1;memcpy(buf,ctl->spec.text+start,len);buf[len]='\0';
                if(buf[0])TTF_SizeUTF8(font,buf,&w,&h);
                if(w>=targetx){
                    if(abs(targetx-pw)<=abs(w-targetx))return prev;
                    return q;
                }
                prev=q;pw=w;p=q;
            }
            return end;
        }
        if(next<=start)break;
        start=next;row++;
    }
    return (uint32_t)strlen(ctl->spec.text);
}

static void host_ui_tarea_caret_visual(HostUiControl *ctl,SDL_Rect r,int *out_x,int *out_row){
    TTF_Font *font;
    uint32_t caret,start=0,end=0,next=0,row=0,maxw;
    int w=0,h=0;
    if(out_x)*out_x=0;if(out_row)*out_row=0;if(!ctl)return;
    font=host_ui_get_font(ctl->spec.font,ctl->spec.font_size,ctl->spec.bold,ctl->spec.italic,ctl->spec.underline);
    if(!font)return;
    caret=ctl->caret;if(caret>(uint32_t)strlen(ctl->spec.text))caret=(uint32_t)strlen(ctl->spec.text);
    maxw=r.w>(int)(2u*ctl->spec.pad_x)?(uint32_t)(r.w-(int)(2u*ctl->spec.pad_x)):0;
    while(start<=caret){
        if(start==(uint32_t)strlen(ctl->spec.text)){end=next=start;}
        else if(ctl->spec.wrap)host_ui_tarea_next_line(font,ctl->spec.text,start,maxw,&end,&next);
        else{
            const char *nl=strchr(ctl->spec.text+start,'\n');
            end=nl?(uint32_t)(nl-ctl->spec.text):(uint32_t)strlen(ctl->spec.text);
            next=nl?end+1:end;
        }
        if(caret<=end){
            char buf[NYOTA_UI_TEXT_MAX];size_t len=caret-start;
            if(len>=sizeof(buf))len=sizeof(buf)-1;memcpy(buf,ctl->spec.text+start,len);buf[len]='\0';
            if(buf[0])TTF_SizeUTF8(font,buf,&w,&h);
            if(out_x)*out_x=w;if(out_row)*out_row=(int)row;return;
        }
        if(next<=start)break;
        start=next;row++;
    }
}

static void host_ui_draw_tarea(SDL_Renderer *ren,HostUiControl *ctl,SDL_Rect r,int idx){
    TTF_Font *font;
    SDL_Color col;
    uint32_t start=0,end=0,next=0,row=0,maxw;
    int lineh;
    if(!ren||!ctl)return;
    font=host_ui_get_font(ctl->spec.font,ctl->spec.font_size,ctl->spec.bold,ctl->spec.italic,ctl->spec.underline);
    if(!font){host_ui_draw_text(ren,&ctl->spec,r);return;}
    lineh=TTF_FontLineSkip(font);if(lineh<=0)lineh=(int)ctl->spec.font_size+2;
    maxw=r.w>(int)(2u*ctl->spec.pad_x)?(uint32_t)(r.w-(int)(2u*ctl->spec.pad_x)):0;
    col.r=ctl->spec.text_color.r;col.g=ctl->spec.text_color.g;col.b=ctl->spec.text_color.b;col.a=ctl->spec.text_color.a;
    while(start<(uint32_t)strlen(ctl->spec.text) || (start==0&&!ctl->spec.text[0])){
        char line[NYOTA_UI_TEXT_MAX];
        size_t len;
        SDL_Surface *sf;SDL_Texture *tx;SDL_Rect dst;
        if(!ctl->spec.text[0]){end=next=0;}else if(ctl->spec.wrap)host_ui_tarea_next_line(font,ctl->spec.text,start,maxw,&end,&next);
        else{
            const char *nl=strchr(ctl->spec.text+start,'\n');
            end=nl?(uint32_t)(nl-ctl->spec.text):(uint32_t)strlen(ctl->spec.text);
            next=nl?end+1:end;
        }
        len=end-start;if(len>=sizeof(line))len=sizeof(line)-1;memcpy(line,ctl->spec.text+start,len);line[len]='\0';
        if(line[0]){
            sf=TTF_RenderUTF8_Blended(font,line,col);
            if(sf){
                dst.x=r.x+(int)ctl->spec.pad_x;dst.y=r.y+(int)ctl->spec.pad_y+(int)row*lineh;dst.w=sf->w;dst.h=sf->h;
                host_ui_apply_ancestor_mask(idx,sf,dst.x,dst.y);
                tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);
                if(tx){SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);SDL_RenderCopy(ren,tx,NULL,&dst);SDL_DestroyTexture(tx);}
            }
        }
        if(next<=start)break;
        start=next;row++;
        if(r.y+(int)ctl->spec.pad_y+(int)row*lineh>=r.y+r.h)break;
    }
    if(ctl->focused&&ctl->spec.enabled){
        uint32_t caret=ctl->caret,n=(uint32_t)strlen(ctl->spec.text),p=0,ls=0,le=0,ln=0,ri=0;
        int tw=0,th=0,cx,cy;
        if(caret>n)caret=n;
        while(p<caret){
            uint32_t e,nx;
            if(ctl->spec.wrap)host_ui_tarea_next_line(font,ctl->spec.text,p,maxw,&e,&nx);
            else{const char *nl=strchr(ctl->spec.text+p,'\n');e=nl?(uint32_t)(nl-ctl->spec.text):n;nx=nl?e+1:e;}
            if(caret<=e){ls=p;le=caret;ri=ln;break;}
            if(caret<nx){ls=nx;le=nx;ri=ln+1;break;}
            p=nx;ln++;ls=p;le=p;ri=ln;
        }
        if(caret==0){ls=le=ri=0;}
        else if(p>=caret){ls=p;le=caret;ri=ln;}
        {
            char prefix[NYOTA_UI_TEXT_MAX];size_t plen=le>ls?le-ls:0;
            if(plen>=sizeof(prefix))plen=sizeof(prefix)-1;memcpy(prefix,ctl->spec.text+ls,plen);prefix[plen]='\0';
            if(prefix[0])TTF_SizeUTF8(font,prefix,&tw,&th);
        }
        cx=r.x+(int)ctl->spec.pad_x+tw;
        cy=r.y+(int)ctl->spec.pad_y+(int)ri*lineh;
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren,ctl->spec.caret_color.r,ctl->spec.caret_color.g,ctl->spec.caret_color.b,ctl->spec.caret_color.a);
        if(host_ui_point_in_rounded_ancestor(idx,cx+0.5,cy+0.5))
            host_ui_draw_line_masked(ren,idx,cx,cy,cx,cy+lineh-2);
    }
}

static void host_ui_fill_circle(SDL_Renderer *ren,int idx,SDL_Rect q){
    int cy=q.y+q.h/2,rx=q.w/2,ry=q.h/2,y;
    if(!ren||q.w<=0||q.h<=0||rx<=0||ry<=0)return;
    for(y=q.y;y<q.y+q.h;y++){
        double dy=((double)y+0.5-(double)cy)/(double)ry;
        double f=1.0-dy*dy;
        int dx;
        if(f<0.0)continue;
        dx=(int)floor((double)rx*sqrt(f));
        host_ui_draw_line_masked(ren,idx,q.x+rx-dx,y,q.x+rx+dx,y);
    }
}

static void host_ui_fill_diamond(SDL_Renderer *ren,int idx,SDL_Rect q){
    int cy=q.y+q.h/2,cx=q.x+q.w/2,hh=q.h/2,y;
    if(!ren||q.w<=0||q.h<=0||hh<=0)return;
    for(y=q.y;y<q.y+q.h;y++){
        int dy=abs(y-cy);
        int dx=(int)(((int64_t)(hh-dy)*(int64_t)(q.w/2))/hh);
        if(dx<0)continue;
        host_ui_draw_line_masked(ren,idx,cx-dx,y,cx+dx,y);
    }
}

static void host_ui_fill_triangle(SDL_Renderer *ren,int idx,SDL_Rect q,uint8_t orientation){
    int i;
    if(!ren||q.w<=0||q.h<=0)return;
    if(orientation==NYOTA_UI_SEP_VERTICAL){
        for(i=0;i<q.h;i++){
            int half=(int)(((int64_t)(i+1)*(q.w/2))/q.h);
            int cx=q.x+q.w/2;
            host_ui_draw_line_masked(ren,idx,cx-half,q.y+i,cx+half,q.y+i);
        }
    }else{
        int cy=q.y+q.h/2;
        for(i=0;i<q.w;i++){
            int half=(int)(((int64_t)(i+1)*(q.h/2))/q.w);
            host_ui_draw_line_masked(ren,idx,q.x+i,cy-half,q.x+i,cy+half);
        }
    }
}

static void host_ui_fill_parallelogram(SDL_Renderer *ren,int idx,SDL_Rect q){
    int y,skew;
    if(!ren||q.w<=0||q.h<=0)return;
    skew=q.w/5;if(skew<1)skew=1;if(skew>q.h)skew=q.h;
    for(y=0;y<q.h;y++){
        int shift=(int)(((int64_t)(q.h-1-y)*(int64_t)skew)/(q.h?q.h:1));
        int x1=q.x+shift;
        int x2=q.x+q.w-1-(skew-shift);
        if(x2>=x1)host_ui_draw_line_masked(ren,idx,x1,q.y+y,x2,q.y+y);
    }
}

static void host_ui_fill_round_rect(SDL_Renderer *ren,int idx,SDL_Rect q){
    int rad,y;
    if(!ren||q.w<=0||q.h<=0)return;
    rad=(q.w<q.h?q.w:q.h)/2;
    if(rad<=1){host_ui_fill_rect_masked(ren,idx,q);return;}
    for(y=0;y<q.h;y++){
        int inset=0;
        if(y<rad){
            double dy=(double)(rad-y)-0.5;
            inset=rad-(int)floor(sqrt((double)rad*(double)rad-dy*dy));
        }else if(y>=q.h-rad){
            double dy=(double)(y-(q.h-rad))+0.5;
            inset=rad-(int)floor(sqrt((double)rad*(double)rad-dy*dy));
        }
        host_ui_draw_line_masked(ren,idx,q.x+inset,q.y+y,q.x+q.w-1-inset,q.y+y);
    }
}

static void host_ui_draw_sbar_thumb(SDL_Renderer *ren,int idx,HostUiControl *ctl){
    SDL_Rect q;
    NyotaColor c;
    if(!host_ui_range_thumb_rect(idx,&q))return;
    c=(ctl->hover||ctl->pressed)?ctl->spec.thumb_hover_color:ctl->spec.thumb_color;
    if(!ctl->spec.enabled)c=host_ui_tint_color(c,-60);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
    if(ctl->spec.thumb_shape==NYOTA_UI_SBAR_THUMB_RECT)host_ui_fill_rect_masked(ren,idx,q);
    else if(ctl->spec.thumb_shape==NYOTA_UI_SBAR_THUMB_ROUND)host_ui_fill_round_rect(ren,idx,q);
    else if(ctl->spec.thumb_shape==NYOTA_UI_SBAR_THUMB_CIRCLE){
        int d=q.w<q.h?q.w:q.h;SDL_Rect z={q.x+(q.w-d)/2,q.y+(q.h-d)/2,d,d};host_ui_fill_circle(ren,idx,z);
    }else if(ctl->spec.thumb_shape==NYOTA_UI_SBAR_THUMB_DIAMOND){
        int d=q.w<q.h?q.w:q.h;SDL_Rect z={q.x+(q.w-d)/2,q.y+(q.h-d)/2,d,d};host_ui_fill_diamond(ren,idx,z);
    }else if(ctl->spec.thumb_shape==NYOTA_UI_SBAR_THUMB_TRIANGLE)host_ui_fill_triangle(ren,idx,q,ctl->spec.orientation);
    else host_ui_fill_parallelogram(ren,idx,q);
}

static void host_ui_draw_slider_thumb(SDL_Renderer *ren,int idx,HostUiControl *ctl){
    host_ui_draw_sbar_thumb(ren,idx,ctl);
}

static void host_ui_draw_pbar(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    uint32_t span=ctl->spec.range_max-ctl->spec.range_min;
    if(ctl->spec.border&&ctl->spec.border_width){
        int p=(int)ctl->spec.border_width;
        r.x+=p;r.y+=p;r.w-=2*p;r.h-=2*p;
        if(r.w<=0||r.h<=0)return;
    }
    uint32_t pos=ctl->spec.range_value-ctl->spec.range_min;
    NyotaColor fill=ctl->spec.progress_fill_color,empty=ctl->spec.progress_empty_color;
    if(!span)return;
    if(ctl->spec.progress_shape==NYOTA_UI_PBAR_SHAPE_BAR){
        SDL_Rect q=r;
        if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){
            q.h=(int)(((uint64_t)r.h*(uint64_t)pos)/(uint64_t)span);
            q.y=r.y+r.h-q.h;
        }else q.w=(int)(((uint64_t)r.w*(uint64_t)pos)/(uint64_t)span);
        SDL_SetRenderDrawColor(ren,fill.r,fill.g,fill.b,fill.a);
        host_ui_fill_rect_masked(ren,idx,q);
        return;
    }
    {
        uint32_t seg=ctl->spec.progress_segments?ctl->spec.progress_segments:1;
        uint32_t active=(uint32_t)(((uint64_t)pos*(uint64_t)seg)/(uint64_t)span);
        uint32_t gap=ctl->spec.progress_gap,k;
        int total=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?r.h:r.w;
        int cross=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?r.w:r.h;
        int slot;
        if(pos==span)active=seg;
        if(seg>1&&gap*(seg-1)>=(uint32_t)total)gap=0;
        slot=(total-(int)(gap*(seg>0?seg-1:0)))/(int)seg;
        if(slot<1)slot=1;
        for(k=0;k<seg;k++){
            SDL_Rect q;NyotaColor c=k<active?fill:empty;
            if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){
                q.x=r.x;q.w=cross;q.h=slot;
                q.y=r.y+r.h-(int)((k+1)*slot+k*gap);
            }else{
                q.x=r.x+(int)(k*(slot+gap));q.y=r.y;q.w=slot;q.h=cross;
            }
            SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
            if(ctl->spec.progress_shape==NYOTA_UI_PBAR_SHAPE_CIRCLE){
                int d=q.w<q.h?q.w:q.h;SDL_Rect z={q.x+(q.w-d)/2,q.y+(q.h-d)/2,d,d};host_ui_fill_circle(ren,idx,z);
            }else if(ctl->spec.progress_shape==NYOTA_UI_PBAR_SHAPE_TRIANGLE)host_ui_fill_triangle(ren,idx,q,ctl->spec.orientation);
            else if(ctl->spec.progress_shape==NYOTA_UI_PBAR_SHAPE_SQUARE){
                int d=q.w<q.h?q.w:q.h;SDL_Rect z={q.x+(q.w-d)/2,q.y+(q.h-d)/2,d,d};host_ui_fill_rect_masked(ren,idx,z);
            }else host_ui_fill_parallelogram(ren,idx,q);
        }
    }
}

static void host_ui_clear_eq_cache(HostUiControl *ctl){
    if(!ctl)return;
    if(ctl->eq_barbg_cache)SDL_DestroyTexture(ctl->eq_barbg_cache);
    if(ctl->eq_fill_cache)SDL_DestroyTexture(ctl->eq_fill_cache);
    if(ctl->eq_glow_cache)SDL_DestroyTexture(ctl->eq_glow_cache);
    ctl->eq_barbg_cache=ctl->eq_fill_cache=ctl->eq_glow_cache=NULL;
    ctl->eq_cache_w=ctl->eq_cache_h=0;
}

static void host_ui_clear_asset_cache(HostUiControl *ctl){
    if(!ctl)return;
    if(ctl->icon_cache){SDL_DestroyTexture(ctl->icon_cache);ctl->icon_cache=NULL;}
    if(ctl->tree_leaf_cache){SDL_DestroyTexture(ctl->tree_leaf_cache);ctl->tree_leaf_cache=NULL;}
    if(ctl->tree_closed_cache){SDL_DestroyTexture(ctl->tree_closed_cache);ctl->tree_closed_cache=NULL;}
    if(ctl->tree_open_cache){SDL_DestroyTexture(ctl->tree_open_cache);ctl->tree_open_cache=NULL;}
}

static int host_ui_bg_is_transparent(const NyotaUiBackground *bg){
    return bg&&bg->kind==NYOTA_UI_BG_COLOR&&bg->colors[0].mode==NYOTA_COLOR_TRANSPARENT;
}

static SDL_Texture *host_ui_background_texture(SDL_Renderer *ren,const NyotaUiBackground *bg,uint32_t w,uint32_t h){
    NyotaUiControlSpec ts;SDL_Surface *sf;SDL_Texture *tx;
    if(!ren||!bg||!w||!h||host_ui_bg_is_transparent(bg))return NULL;
    memset(&ts,0,sizeof(ts));ts.kind=NYOTA_UI_CTRL_PANEL;ts.w=w;ts.h=h;ts.background=*bg;
    sf=host_ui_control_background_surface(&ts,bg);if(!sf)return NULL;
    tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);
    if(tx)SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);
    return tx;
}

static void host_ui_eqbox_ensure_cache(SDL_Renderer *ren,HostUiControl *ctl,uint32_t w,uint32_t h){
    if(!ctl||!ren||!w||!h)return;
    if(ctl->eq_cache_w==w&&ctl->eq_cache_h==h&&
       (ctl->eq_barbg_cache||host_ui_bg_is_transparent(&ctl->spec.eq_bar_background))&&
       (ctl->eq_fill_cache||host_ui_bg_is_transparent(&ctl->spec.eq_bar_fill))&&
       (ctl->eq_glow_cache||host_ui_bg_is_transparent(&ctl->spec.eq_glow)))return;
    host_ui_clear_eq_cache(ctl);
    ctl->eq_cache_w=w;ctl->eq_cache_h=h;
    ctl->eq_barbg_cache=host_ui_background_texture(ren,&ctl->spec.eq_bar_background,w,h);
    ctl->eq_fill_cache=host_ui_background_texture(ren,&ctl->spec.eq_bar_fill,w,h);
    ctl->eq_glow_cache=host_ui_background_texture(ren,&ctl->spec.eq_glow,w,h);
}

static NyotaColor host_ui_eq_sample(const NyotaUiBackground *bg,double t){
    NyotaColor c;uint8_t r=255,g=255,b=255,a=255;
    memset(&c,0,sizeof(c));c.mode=NYOTA_COLOR_SOLID;
    if(!bg)return c;
    if(bg->kind==NYOTA_UI_BG_COLOR){
        c=bg->colors[0];
        if(c.mode==NYOTA_COLOR_TRANSPARENT){c.r=c.g=c.b=0;c.a=0;c.mode=NYOTA_COLOR_SOLID;}
        return c;
    }
    if(bg->kind!=NYOTA_UI_BG_IMAGE){
        if(t<0.0)t=0.0;if(t>1.0)t=1.0;
        host_ui_lerp_color(bg,t,&r,&g,&b,&a);c.r=r;c.g=g;c.b=b;c.a=a;return c;
    }
    c.r=c.g=c.b=255;c.a=255;return c;
}

static SDL_Rect host_ui_eq_active_rect(const HostUiControl *ctl,SDL_Rect track,uint32_t value){
    SDL_Rect q=track;uint32_t span=ctl->spec.range_max-ctl->spec.range_min,pos;
    int total=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?track.h:track.w;
    int minlen=(int)ctl->spec.eq_min_height,maxlen=ctl->spec.eq_max_height?(int)ctl->spec.eq_max_height:total,len;
    if(maxlen>total)maxlen=total;if(minlen>maxlen)minlen=maxlen;
    pos=value<=ctl->spec.range_min?0:(value>=ctl->spec.range_max?span:value-ctl->spec.range_min);
    len=minlen+(span?(int)(((uint64_t)(maxlen-minlen)*(uint64_t)pos)/(uint64_t)span):0);
    if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){
        q.h=len;
        if(ctl->spec.eq_direction==NYOTA_UI_EQ_DOWN)q.y=track.y;
        else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER)q.y=track.y+(track.h-len)/2;
        else q.y=track.y+track.h-len;
    }else{
        q.w=len;
        if(ctl->spec.eq_direction==NYOTA_UI_EQ_RIGHT)q.x=track.x;
        else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER)q.x=track.x+(track.w-len)/2;
        else q.x=track.x+track.w-len;
    }
    return q;
}

static void host_ui_eq_copy_active(SDL_Renderer *ren,SDL_Texture *tx,SDL_Rect track,SDL_Rect active,uint8_t orientation,uint8_t direction){
    SDL_Rect src;
    if(!tx||active.w<=0||active.h<=0)return;
    src.x=active.x-track.x;src.y=active.y-track.y;src.w=active.w;src.h=active.h;
    if(orientation==NYOTA_UI_SEP_VERTICAL&&(direction==NYOTA_UI_EQ_UP||direction==NYOTA_UI_EQ_CENTER))src.y=active.y-track.y;
    if(orientation==NYOTA_UI_SEP_HORIZONTAL&&(direction==NYOTA_UI_EQ_LEFT||direction==NYOTA_UI_EQ_CENTER))src.x=active.x-track.x;
    SDL_RenderCopy(ren,tx,&src,&active);
}

static void host_ui_eq_draw_glow(SDL_Renderer *ren,HostUiControl *ctl,SDL_Rect track,SDL_Rect active){
    uint32_t blur=ctl->spec.eq_blur;int pass;
    if(!blur||!ctl->eq_glow_cache||active.w<=0||active.h<=0)return;
    for(pass=3;pass>=1;pass--){
        int grow=(int)((blur*(uint32_t)pass)/3u);
        SDL_Rect d={active.x-grow,active.y-grow,active.w+2*grow,active.h+2*grow};
        Uint8 alpha=(Uint8)(36+(3-pass)*28);
        SDL_SetTextureAlphaMod(ctl->eq_glow_cache,alpha);
        SDL_RenderCopy(ren,ctl->eq_glow_cache,NULL,&d);
    }
    SDL_SetTextureAlphaMod(ctl->eq_glow_cache,255);
    (void)track;
}

static void host_ui_eq_draw_segment_shape(SDL_Renderer *ren,int idx,uint8_t build,SDL_Rect q,NyotaColor c,uint8_t orientation){
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
    if(build==NYOTA_UI_EQ_CIRCLE)host_ui_fill_circle(ren,idx,q);
    else if(build==NYOTA_UI_EQ_SQUARE){int d=q.w<q.h?q.w:q.h;SDL_Rect z={q.x+(q.w-d)/2,q.y+(q.h-d)/2,d,d};host_ui_fill_rect_masked(ren,idx,z);}
    else if(build==NYOTA_UI_EQ_TRIANGLE)host_ui_fill_triangle(ren,idx,q,orientation);
    else if(build==NYOTA_UI_EQ_DIAMOND)host_ui_fill_diamond(ren,idx,q);
    else if(build==NYOTA_UI_EQ_PARALLELOGRAM)host_ui_fill_parallelogram(ren,idx,q);
    else host_ui_fill_rect_masked(ren,idx,q);
}


static int host_ui_eq_segment_layout(const HostUiControl *ctl,SDL_Rect track,int *segments,int *size,int *gap,int *group_len){
    int len,seg,sz,sg,total,maxsz;
    if(!ctl||!segments||!size||!gap||!group_len)return 0;
    len=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?track.h:track.w;
    if(len<=0)return 0;
    seg=(int)ctl->spec.eq_segments;if(seg<1)seg=1;if(seg>256)seg=256;
    sz=(int)ctl->spec.eq_segment_size;if(sz<2)sz=2;
    sg=(int)ctl->spec.eq_segment_gap;if(sg<0)sg=0;
    total=seg*sz+(seg>1?(seg-1)*sg:0);
    if(total>len){
        maxsz=(len-(seg>1?(seg-1)*sg:0))/seg;
        if(maxsz<2){
            sg=seg>1?(len-2*seg)/(seg-1):0;if(sg<0)sg=0;
            maxsz=(len-(seg>1?(seg-1)*sg:0))/seg;
        }
        if(maxsz<2){
            seg=(len+sg)/(2+sg);if(seg<1)seg=1;
            maxsz=(len-(seg>1?(seg-1)*sg:0))/seg;
        }
        if(maxsz<2)maxsz=2;
        if(sz>maxsz)sz=maxsz;
    }
    total=seg*sz+(seg>1?(seg-1)*sg:0);
    if(total>len&&seg==1){sz=len;total=len;}
    *segments=seg;*size=sz;*gap=sg;*group_len=total;
    return seg>0&&sz>0;
}

static SDL_Rect host_ui_eq_segment_rect(const HostUiControl *ctl,SDL_Rect track,int physical,int segments,int size,int gap,int group_len){
    SDL_Rect q={0,0,0,0};int start,cross;
    if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){
        if(ctl->spec.eq_direction==NYOTA_UI_EQ_DOWN)start=track.y;
        else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER)start=track.y+(track.h-group_len)/2;
        else start=track.y+track.h-group_len;
        cross=size;if(cross>track.w)cross=track.w;if(cross<1)cross=1;
        q.x=track.x+(track.w-cross)/2;q.w=cross;
        q.y=start+physical*(size+gap);q.h=size;
    }else{
        if(ctl->spec.eq_direction==NYOTA_UI_EQ_RIGHT)start=track.x;
        else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER)start=track.x+(track.w-group_len)/2;
        else start=track.x+track.w-group_len;
        cross=size;if(cross>track.h)cross=track.h;if(cross<1)cross=1;
        q.y=track.y+(track.h-cross)/2;q.h=cross;
        q.x=start+physical*(size+gap);q.w=size;
    }
    (void)segments;
    return q;
}

static double host_ui_eq_segment_t(const HostUiControl *ctl,int physical,int segments){
    double t;
    if(segments<=1)return 0.0;
    t=(double)physical/(double)(segments-1);
    if(ctl->spec.eq_direction==NYOTA_UI_EQ_UP||ctl->spec.eq_direction==NYOTA_UI_EQ_LEFT)t=1.0-t;
    else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER){
        double center=(double)(segments-1)/2.0,dist=fabs((double)physical-center),den=center>0.0?center:1.0;
        t=dist/den;
    }
    return t;
}

static uint32_t host_ui_eq_active_segment_count(const HostUiControl *ctl,uint32_t value,int segments){
    uint32_t span,pos;
    if(!ctl||segments<=0||ctl->spec.range_max<=ctl->spec.range_min)return 0;
    span=ctl->spec.range_max-ctl->spec.range_min;
    if(value<=ctl->spec.range_min)return 0;
    if(value>=ctl->spec.range_max)return (uint32_t)segments;
    pos=value-ctl->spec.range_min;
    return (uint32_t)(((uint64_t)pos*(uint64_t)segments+(uint64_t)span-1u)/(uint64_t)span);
}

static int host_ui_eq_segment_active(const HostUiControl *ctl,int physical,int segments,uint32_t active){
    int start;
    if(active==0)return 0;if(active>=(uint32_t)segments)return 1;
    if(ctl->spec.eq_direction==NYOTA_UI_EQ_UP||ctl->spec.eq_direction==NYOTA_UI_EQ_LEFT)
        return physical>=segments-(int)active;
    if(ctl->spec.eq_direction==NYOTA_UI_EQ_DOWN||ctl->spec.eq_direction==NYOTA_UI_EQ_RIGHT)
        return physical<(int)active;
    start=(segments-(int)active)/2;
    return physical>=start&&physical<start+(int)active;
}

static int host_ui_eq_peak_physical(const HostUiControl *ctl,int segments,uint32_t active){
    int start;
    if(!active||segments<=0)return -1;
    if(active>(uint32_t)segments)active=(uint32_t)segments;
    if(ctl->spec.eq_direction==NYOTA_UI_EQ_UP||ctl->spec.eq_direction==NYOTA_UI_EQ_LEFT)
        return segments-(int)active;
    if(ctl->spec.eq_direction==NYOTA_UI_EQ_DOWN||ctl->spec.eq_direction==NYOTA_UI_EQ_RIGHT)
        return (int)active-1;
    start=(segments-(int)active)/2;
    return start+(int)active-1;
}

static void host_ui_eq_draw_segment_glow(SDL_Renderer *ren,int idx,uint8_t build,SDL_Rect q,NyotaColor c,uint8_t orientation,uint32_t blur){
    int pass;
    if(!blur||c.a==0||c.mode==NYOTA_COLOR_TRANSPARENT)return;
    for(pass=3;pass>=1;pass--){
        int grow=(int)((blur*(uint32_t)pass)/3u);SDL_Rect gq=q;NyotaColor gc=c;
        gq.x-=grow;gq.y-=grow;gq.w+=2*grow;gq.h+=2*grow;
        gc.a=(uint8_t)(18+(3-pass)*18);
        host_ui_eq_draw_segment_shape(ren,idx,build,gq,gc,orientation);
    }
}

static uint32_t host_ui_eq_peak_value(HostUiControl *ctl,uint32_t bar,uint32_t value){
    uint64_t now,hold_ticks;
    if(!ctl||bar>=NYOTA_UI_EQ_MAX_BARS)return value;
    if(!ctl->spec.eq_peak)return value;
    now=host_ticks();
    if(ctl->eq_peak_value[bar]<ctl->spec.range_min||ctl->eq_peak_value[bar]>ctl->spec.range_max||
       value>=ctl->eq_peak_value[bar]){
        ctl->eq_peak_value[bar]=value;
        ctl->eq_peak_tick[bar]=now;
    }else{
        hold_ticks=((uint64_t)ctl->spec.eq_peak_hold+9u)/10u;
        if(ctl->spec.eq_peak_hold==0||now-ctl->eq_peak_tick[bar]>=hold_ticks){
            ctl->eq_peak_value[bar]=value;
            ctl->eq_peak_tick[bar]=now;
        }
    }
    return ctl->eq_peak_value[bar];
}

static void host_ui_eq_draw_solid_peak(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect track,uint32_t peak){
    SDL_Rect p=host_ui_eq_active_rect(ctl,track,peak);NyotaColor c=ctl->spec.eq_peak_color;
    if(p.w<=0||p.h<=0||peak<=ctl->spec.range_min)return;
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
    if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){
        int y;
        if(ctl->spec.eq_direction==NYOTA_UI_EQ_DOWN)y=p.y+p.h-2;
        else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER)y=p.y;
        else y=p.y;
        {SDL_Rect q={track.x,y,track.w,2};host_ui_fill_rect_masked(ren,idx,q);}
    }else{
        int x;
        if(ctl->spec.eq_direction==NYOTA_UI_EQ_RIGHT)x=p.x+p.w-2;
        else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER)x=p.x+p.w-2;
        else x=p.x;
        {SDL_Rect q={x,track.y,2,track.h};host_ui_fill_rect_masked(ren,idx,q);}
    }
}

static void host_ui_draw_eqbox(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    SDL_Rect inner=r,label_strip={0,0,0,0};uint32_t n=ctl->spec.eq_bars,i;int bw,gap=(int)ctl->spec.eq_gap;
    int border=ctl->spec.border?(int)ctl->spec.border_width:0;
    int total,start,crossLen,label_h=0;
    if(border){inner.x+=border;inner.y+=border;inner.w-=2*border;inner.h-=2*border;}
    inner.x+=(int)ctl->spec.pad_x;inner.w-=2*(int)ctl->spec.pad_x;
    inner.y+=(int)ctl->spec.pad_y;inner.h-=2*(int)ctl->spec.pad_y;
    if(inner.w<=0||inner.h<=0||!n)return;

    if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL&&ctl->spec.eq_show_labels&&
       (ctl->spec.eq_label_pos==NYOTA_UI_EQ_LABEL_BOTTOM||ctl->spec.eq_label_pos==NYOTA_UI_EQ_LABEL_TOP)){
        label_h=(int)ctl->spec.font_size+8;if(label_h<18)label_h=18;if(label_h>inner.h/3)label_h=inner.h/3;
        label_strip=inner;label_strip.h=label_h;
        if(ctl->spec.eq_label_pos==NYOTA_UI_EQ_LABEL_BOTTOM){
            label_strip.y=inner.y+inner.h-label_h;inner.h-=label_h;
        }else{
            inner.y+=label_h;inner.h-=label_h;
        }
    }

    bw=ctl->spec.eq_build==NYOTA_UI_EQ_SOLID?(int)ctl->spec.eq_bar_width:(int)ctl->spec.eq_segment_size;
    if(bw<2)bw=2;
    total=(int)n*bw+(int)(n>1?(n-1u)*(uint32_t)gap:0u);
    crossLen=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?inner.w:inner.h;
    start=(crossLen-total)/2;if(start<0)start=0;

    for(i=0;i<n;i++){
        SDL_Rect track,active;uint32_t v=ctl->spec.eq_values[i],peakv;
        int maxlen;
        if(v<ctl->spec.range_min)v=ctl->spec.range_min;if(v>ctl->spec.range_max)v=ctl->spec.range_max;
        if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){
            maxlen=ctl->spec.eq_max_height?(int)ctl->spec.eq_max_height:inner.h;if(maxlen>inner.h)maxlen=inner.h;
            track.x=inner.x+start+(int)i*(bw+gap);track.w=bw;track.h=maxlen;
            if(ctl->spec.eq_direction==NYOTA_UI_EQ_DOWN)track.y=inner.y;
            else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER)track.y=inner.y+(inner.h-maxlen)/2;
            else track.y=inner.y+inner.h-maxlen;
        }else{
            maxlen=ctl->spec.eq_max_height?(int)ctl->spec.eq_max_height:inner.w;if(maxlen>inner.w)maxlen=inner.w;
            track.y=inner.y+start+(int)i*(bw+gap);track.h=bw;track.w=maxlen;
            if(ctl->spec.eq_direction==NYOTA_UI_EQ_RIGHT)track.x=inner.x;
            else if(ctl->spec.eq_direction==NYOTA_UI_EQ_CENTER)track.x=inner.x+(inner.w-maxlen)/2;
            else track.x=inner.x+inner.w-maxlen;
        }
        peakv=host_ui_eq_peak_value(ctl,i,v);

        if(ctl->spec.eq_build==NYOTA_UI_EQ_SOLID){
            host_ui_eqbox_ensure_cache(ren,ctl,(uint32_t)track.w,(uint32_t)track.h);
            if(ctl->eq_barbg_cache)SDL_RenderCopy(ren,ctl->eq_barbg_cache,NULL,&track);
            active=host_ui_eq_active_rect(ctl,track,v);
            host_ui_eq_draw_glow(ren,ctl,track,active);
            if(i<ctl->spec.eq_color_count&&ctl->spec.eq_bar_color_set[i]){
                NyotaColor cc=ctl->spec.eq_bar_colors[i];SDL_SetRenderDrawColor(ren,cc.r,cc.g,cc.b,cc.a);host_ui_fill_rect_masked(ren,idx,active);
            }else if(ctl->eq_fill_cache)host_ui_eq_copy_active(ren,ctl->eq_fill_cache,track,active,ctl->spec.orientation,ctl->spec.eq_direction);
            if(ctl->spec.eq_peak)host_ui_eq_draw_solid_peak(ren,idx,ctl,track,peakv);
        }else{
            int segments,size,sg,group_len,j;uint32_t active_count,peak_count;int peak_index=-1;
            if(host_ui_eq_segment_layout(ctl,track,&segments,&size,&sg,&group_len)){
                active_count=host_ui_eq_active_segment_count(ctl,v,segments);
                peak_count=host_ui_eq_active_segment_count(ctl,peakv,segments);
                if(ctl->spec.eq_peak)peak_index=host_ui_eq_peak_physical(ctl,segments,peak_count);

                /* Najpierw pelna matryca nieaktywnych segmentow — bez prostokatnego BARBG. */
                for(j=0;j<segments;j++){
                    SDL_Rect q=host_ui_eq_segment_rect(ctl,track,j,segments,size,sg,group_len);
                    double t=host_ui_eq_segment_t(ctl,j,segments);NyotaColor bg=host_ui_eq_sample(&ctl->spec.eq_bar_background,t);
                    bg.a=(uint8_t)(((uint32_t)bg.a*ctl->spec.eq_inactive_alpha)/255u);
                    host_ui_eq_draw_segment_shape(ren,idx,ctl->spec.eq_build,q,bg,ctl->spec.orientation);
                }

                /* Aktywne segmenty zastępują nieaktywne dokładnie tym samym kształtem. */
                for(j=0;j<segments;j++)if(host_ui_eq_segment_active(ctl,j,segments,active_count)){
                    SDL_Rect q=host_ui_eq_segment_rect(ctl,track,j,segments,size,sg,group_len);
                    double t=host_ui_eq_segment_t(ctl,j,segments);NyotaColor cc,gc;
                    cc=(i<ctl->spec.eq_color_count&&ctl->spec.eq_bar_color_set[i])?ctl->spec.eq_bar_colors[i]:host_ui_eq_sample(&ctl->spec.eq_bar_fill,t);
                    gc=host_ui_eq_sample(&ctl->spec.eq_glow,t);
                    host_ui_eq_draw_segment_glow(ren,idx,ctl->spec.eq_build,q,gc,ctl->spec.orientation,ctl->spec.eq_blur);
                    host_ui_eq_draw_segment_shape(ren,idx,ctl->spec.eq_build,q,cc,ctl->spec.orientation);
                }

                if(peak_index>=0&&peak_index<segments){
                    SDL_Rect pq=host_ui_eq_segment_rect(ctl,track,peak_index,segments,size,sg,group_len);
                    host_ui_eq_draw_segment_shape(ren,idx,ctl->spec.eq_build,pq,ctl->spec.eq_peak_color,ctl->spec.orientation);
                }
            }
            active=host_ui_eq_active_rect(ctl,track,v);
        }

        if(ctl->spec.eq_show_labels&&i<ctl->spec.eq_label_count&&ctl->spec.eq_labels[i][0]){
            NyotaUiControlSpec ts=ctl->spec;SDL_Rect lr=track;
            strncpy(ts.text,ctl->spec.eq_labels[i],sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';
            ts.halign=NYOTA_UI_ALIGN_CENTER;ts.valign=NYOTA_UI_VALIGN_MIDDLE;ts.pad_x=ts.pad_y=0;
            if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL&&label_h){
                lr.x=track.x-(gap/2);lr.w=track.w+gap;lr.y=label_strip.y;lr.h=label_strip.h;
            }else if(ctl->spec.eq_label_pos==NYOTA_UI_EQ_LABEL_BOTTOM){
                lr.y=track.y+track.h-(int)ctl->spec.font_size-4;lr.h=(int)ctl->spec.font_size+4;
            }else if(ctl->spec.eq_label_pos==NYOTA_UI_EQ_LABEL_TOP)lr.h=(int)ctl->spec.font_size+4;
            else lr=active;
            host_ui_draw_text(ren,&ts,lr);
        }
        if(ctl->spec.eq_show_values){
            NyotaUiControlSpec ts=ctl->spec;SDL_Rect vr=active;char vb[24];
            snprintf(vb,sizeof(vb),"%u",v);strncpy(ts.text,vb,sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';
            ts.halign=NYOTA_UI_ALIGN_CENTER;ts.valign=NYOTA_UI_VALIGN_MIDDLE;ts.pad_x=ts.pad_y=0;
            host_ui_draw_text(ren,&ts,vr);
        }
    }
}


static void host_ui_draw_thick_line(SDL_Renderer *ren,int idx,int x0,int y0,int x1,int y1,NyotaColor c,int width){
    double dx=(double)x1-(double)x0,dy=(double)y1-(double)y0,len=sqrt(dx*dx+dy*dy);
    int k,half;
    if(!ren||width<=0)return;
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
    if(len<0.5){SDL_Rect q={x0-width/2,y0-width/2,width,width};host_ui_fill_rect_masked(ren,idx,q);return;}
    half=width/2;
    for(k=-half;k<=half;k++){
        int ox=(int)lround(-(dy/len)*(double)k),oy=(int)lround((dx/len)*(double)k);
        host_ui_draw_line_masked(ren,idx,x0+ox,y0+oy,x1+ox,y1+oy);
    }
}

static void host_ui_draw_glow_line(SDL_Renderer *ren,int idx,int x0,int y0,int x1,int y1,NyotaColor c,int width,uint32_t blur){
    int p;
    if(!blur||c.mode==NYOTA_COLOR_TRANSPARENT)return;
    for(p=(int)blur;p>=1;p-=2){
        NyotaColor g=c;g.a=(uint8_t)((uint32_t)c.a*(uint32_t)(blur+1u-(uint32_t)p)/(uint32_t)(blur+1u)/3u+12u);
        host_ui_draw_thick_line(ren,idx,x0,y0,x1,y1,g,width+2*p);
    }
}

static void host_ui_polar_point(int cx,int cy,double radius,double deg,int *x,int *y){
    const double pi=3.14159265358979323846;
    double a=deg*pi/180.0;
    if(x)*x=cx+(int)lround(sin(a)*radius);
    if(y)*y=cy-(int)lround(cos(a)*radius);
}

static double host_ui_norm_deg(double a){
    while(a<0.0)a+=360.0;
    while(a>=360.0)a-=360.0;
    return a;
}

static double host_ui_value_t(int32_t v,int32_t minv,int32_t maxv){
    if(maxv<=minv)return 0.0;
    if(v<=minv)return 0.0;if(v>=maxv)return 1.0;
    return (double)((int64_t)v-(int64_t)minv)/(double)((int64_t)maxv-(int64_t)minv);
}

static void host_ui_fill_polygon(SDL_Renderer *ren,int idx,const SDL_Point *pts,int n,NyotaColor c){
    int ymin=2147483647,ymax=-2147483647,y,i,j;
    if(!ren||!pts||n<3)return;
    for(i=0;i<n;i++){if(pts[i].y<ymin)ymin=pts[i].y;if(pts[i].y>ymax)ymax=pts[i].y;}
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
    for(y=ymin;y<=ymax;y++){
        int xs[24],count=0;
        for(i=0,j=n-1;i<n;j=i++){
            int yi=pts[i].y,yj=pts[j].y,xi=pts[i].x,xj=pts[j].x;
            if(((yi<=y&&yj>y)||(yj<=y&&yi>y))&&count<24){
                double t=(double)(y-yi)/(double)(yj-yi);
                xs[count++]=(int)lround((double)xi+t*(double)(xj-xi));
            }
        }
        for(i=0;i<count-1;i++)for(j=i+1;j<count;j++)if(xs[j]<xs[i]){int q=xs[i];xs[i]=xs[j];xs[j]=q;}
        for(i=0;i+1<count;i+=2)host_ui_draw_line_masked(ren,idx,xs[i],y,xs[i+1],y);
    }
}

static SDL_Point host_ui_rot_point(double cx,double cy,double along,double perp,double deg){
    const double pi=3.14159265358979323846;
    double a=deg*pi/180.0;
    double ux=sin(a),uy=-cos(a),px=cos(a),py=sin(a);
    SDL_Point p;
    p.x=(int)lround(cx+ux*along+px*perp);
    p.y=(int)lround(cy+uy*along+py*perp);
    return p;
}

static void host_ui_draw_bg_rect(SDL_Renderer *ren,int idx,SDL_Rect r,const NyotaUiBackground *bg){
    NyotaUiControlSpec ts;SDL_Surface *sf;SDL_Texture *tx;
    if(!ren||!bg||r.w<=0||r.h<=0)return;
    memset(&ts,0,sizeof(ts));ts.kind=NYOTA_UI_CTRL_PANEL;ts.w=(uint32_t)r.w;ts.h=(uint32_t)r.h;ts.background=*bg;
    sf=host_ui_control_background_surface(&ts,bg);if(!sf)return;
    host_ui_apply_ancestor_mask(idx,sf,r.x,r.y);
    tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);
    if(tx){SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);SDL_RenderCopy(ren,tx,NULL,&r);SDL_DestroyTexture(tx);}
}

static void host_ui_draw_tbox(SDL_Renderer *ren,HostUiControl *ctl,SDL_Rect r,int idx){
    NyotaUiControlSpec saved;
    uint32_t oldcaret;
    if(!ren||!ctl)return;
    saved=ctl->spec;oldcaret=ctl->caret;
    if(!ctl->spec.text[0]&&!ctl->focused&&ctl->spec.placeholder[0]){
        ctl->spec.text_color=host_ui_tint_color(ctl->spec.text_color,-76);
        strncpy(ctl->spec.text,ctl->spec.placeholder,sizeof(ctl->spec.text)-1);
        ctl->spec.text[sizeof(ctl->spec.text)-1]='\0';
        host_ui_draw_text(ren,&ctl->spec,r);
    }else if(ctl->spec.password&&ctl->spec.text[0]){
        char mask[NYOTA_UI_TEXT_MAX];uint32_t i=0,n=0,cp=0;
        while(ctl->spec.text[i]&&n+1<sizeof(mask)){
            unsigned char ch=(unsigned char)ctl->spec.text[i];
            if((ch&0xC0u)!=0x80u){mask[n++]='*';if(i<oldcaret)cp++;}
            i++;
        }
        mask[n]='\0';strncpy(ctl->spec.text,mask,sizeof(ctl->spec.text)-1);ctl->spec.text[sizeof(ctl->spec.text)-1]='\0';ctl->caret=cp;
        host_ui_draw_tarea(ren,ctl,r,idx);
    }else host_ui_draw_tarea(ren,ctl,r,idx);
    ctl->spec=saved;ctl->caret=oldcaret;
}

static void host_ui_draw_spinbox(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    char buf[48];SDL_Rect tr=r,up,down;NyotaColor tc=ctl->spec.text_color,ac=ctl->spec.arrow_color;
    int bw=r.h>30?30:r.h;
    snprintf(buf,sizeof(buf),"%d",ctl->spec.signed_value);
    tr.w-=bw;{
        NyotaUiControlSpec ts=ctl->spec;strncpy(ts.text,buf,sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';ts.halign=NYOTA_UI_ALIGN_RIGHT;ts.pad_x=8;
        host_ui_draw_text(ren,&ts,tr);
    }
    up.x=r.x+r.w-bw;up.y=r.y;up.w=bw;up.h=r.h/2;
    down=up;down.y=r.y+r.h/2;down.h=r.h-down.h;
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren,ctl->spec.border_color.r,ctl->spec.border_color.g,ctl->spec.border_color.b,ctl->spec.border_color.a);
    host_ui_draw_line_masked(ren,idx,up.x,down.y,up.x+up.w-1,down.y);
    host_ui_draw_line_masked(ren,idx,up.x,up.y,up.x,up.y+r.h-1);
    if(!ctl->spec.enabled){tc=host_ui_tint_color(tc,-72);ac=host_ui_tint_color(ac,-72);}
    SDL_SetRenderDrawColor(ren,ac.r,ac.g,ac.b,ac.a);
    {int cx=up.x+up.w/2,cy=up.y+up.h/2;host_ui_draw_line_masked(ren,idx,cx-4,cy+2,cx,cy-2);host_ui_draw_line_masked(ren,idx,cx,cy-2,cx+4,cy+2);}
    {int cx=down.x+down.w/2,cy=down.y+down.h/2;host_ui_draw_line_masked(ren,idx,cx-4,cy-2,cx,cy+2);host_ui_draw_line_masked(ren,idx,cx,cy+2,cx+4,cy-2);}
    (void)tc;
}

static uint32_t host_ui_tree_depth_text(const char *s){
    uint32_t d=0;while(s&&*s){if(*s=='/')d++;s++;}return d;
}
static const char *host_ui_tree_leaf(const char *s){const char *p=s,*last=s;while(p&&*p){if(*p=='/')last=p+1;p++;}return last;}
static int host_ui_tree_has_child(const HostUiControl *ctl,uint32_t index){
    char a[NYOTA_UI_TEXT_MAX],b[NYOTA_UI_TEXT_MAX];size_t n;
    if(!ctl||!host_ui_item_at(ctl->spec.items,index,a,sizeof(a))||!host_ui_item_at(ctl->spec.items,index+1,b,sizeof(b)))return 0;
    n=strlen(a);return !strncmp(a,b,n)&&b[n]=='/';
}
static int host_ui_tree_item_visible(const HostUiControl *ctl,uint32_t index){
    char item[NYOTA_UI_TEXT_MAX],parent[NYOTA_UI_TEXT_MAX];uint32_t j;
    if(!ctl||!host_ui_item_at(ctl->spec.items,index,item,sizeof(item)))return 0;
    for(j=0;j<index&&j<64;j++){
        size_t n;
        if(!host_ui_item_at(ctl->spec.items,j,parent,sizeof(parent)))continue;
        n=strlen(parent);
        if(n&&strlen(item)>n&&!strncmp(item,parent,n)&&item[n]=='/'&&!(ctl->tree_expanded_mask&(1ULL<<j)))return 0;
    }
    return 1;
}
static int host_ui_tree_visible_row_to_index(const HostUiControl *ctl,uint32_t row,uint32_t *out){
    uint32_t count=host_ui_combo_item_count(ctl),i,vr=0;
    for(i=0;i<count;i++)if(host_ui_tree_item_visible(ctl,i)){if(vr==row){if(out)*out=i;return 1;}vr++;}
    return 0;
}

static void host_ui_draw_listview(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r,int tree){
    uint32_t count=host_ui_combo_item_count(ctl),i,vr=0;int rowh=(int)ctl->spec.row_height;
    if(rowh<12)rowh=12;
    for(i=0;i<count;i++){
        char item[NYOTA_UI_TEXT_MAX];SDL_Rect row;NyotaColor tc=ctl->spec.text_color;const NyotaUiBackground *rb=NULL;
        uint32_t depth=0;const char *label;
        if(tree&&!host_ui_tree_item_visible(ctl,i))continue;
        row.x=r.x+1;row.y=r.y+1+(int)vr*rowh;row.w=r.w-2;row.h=rowh;
        if(row.y>=r.y+r.h-1)break;
        if(row.y+row.h>r.y+r.h-1)row.h=r.y+r.h-1-row.y;
        if(i==ctl->spec.selected){rb=&ctl->spec.selected_background;tc=ctl->spec.selected_text_color;}
        else if((int32_t)i==ctl->list_hover){rb=&ctl->spec.hover_background;tc=ctl->spec.hover_text_color;}
        if(rb)host_ui_draw_bg_rect(ren,idx,row,rb);
        if(host_ui_item_at(ctl->spec.items,i,item,sizeof(item))){
            SDL_Rect tr=row;
            if(tree){
                depth=host_ui_tree_depth_text(item);label=host_ui_tree_leaf(item);
                tr.x+=(int)(depth*ctl->spec.tree_indent)+18;tr.w-=(int)(depth*ctl->spec.tree_indent)+18;
                if(ctl->spec.show_lines&&depth){
                    int lx=row.x+8+(int)(depth*ctl->spec.tree_indent);
                    SDL_SetRenderDrawColor(ren,ctl->spec.border_color.r,ctl->spec.border_color.g,ctl->spec.border_color.b,110);
                    host_ui_draw_line_masked(ren,idx,lx,row.y,lx,row.y+row.h-1);
                }
                if(host_ui_tree_has_child(ctl,i)){
                    int ax=row.x+8+(int)(depth*ctl->spec.tree_indent),cy=row.y+row.h/2;
                    SDL_SetRenderDrawColor(ren,tc.r,tc.g,tc.b,tc.a);
                    host_ui_draw_line_masked(ren,idx,ax-4,cy,ax+4,cy);
                    if(!(ctl->tree_expanded_mask&(1ULL<<i)))host_ui_draw_line_masked(ren,idx,ax,cy-4,ax,cy+4);
                }
            }else label=item;
            {NyotaUiControlSpec ts=ctl->spec;strncpy(ts.text,label,sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';ts.halign=NYOTA_UI_ALIGN_LEFT;ts.valign=NYOTA_UI_VALIGN_MIDDLE;ts.text_color=tc;host_ui_draw_text(ren,&ts,tr);}
        }
        vr++;
    }
}

static int host_ui_splitter_bar_rect(int idx,SDL_Rect *bar){
    HostUiControl *ctl;SDL_Rect r;uint32_t span,pos;int avail,t;
    if(!bar||idx<0||idx>=HOST_MAX_UI_CONTROLS)return 0;ctl=&g_host_ui_controls[idx];
    if(!ctl->used||ctl->spec.kind!=NYOTA_UI_CTRL_SPLITTER||!host_ui_control_rect_index(idx,&r)||ctl->spec.range_max<=ctl->spec.range_min)return 0;
    span=ctl->spec.range_max-ctl->spec.range_min;t=(int)ctl->spec.splitter_thickness;if(t<1)t=1;
    if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){
        avail=r.w-t;if(avail<0)avail=0;pos=(uint32_t)(((uint64_t)(ctl->spec.range_value-ctl->spec.range_min)*(uint64_t)avail)/(uint64_t)span);
        bar->x=r.x+(int)pos;bar->y=r.y;bar->w=t;bar->h=r.h;
    }else{
        avail=r.h-t;if(avail<0)avail=0;pos=(uint32_t)(((uint64_t)(ctl->spec.range_value-ctl->spec.range_min)*(uint64_t)avail)/(uint64_t)span);
        bar->x=r.x;bar->y=r.y+(int)pos;bar->w=r.w;bar->h=t;
    }
    return 1;
}

static uint32_t host_ui_splitter_value_from_pointer(int idx,int x,int y){
    HostUiControl *ctl=&g_host_ui_controls[idx];SDL_Rect r;int t,avail,p;uint32_t span;
    if(!host_ui_control_rect_index(idx,&r)||ctl->spec.range_max<=ctl->spec.range_min)return ctl->spec.range_value;
    t=(int)ctl->spec.splitter_thickness;span=ctl->spec.range_max-ctl->spec.range_min;
    if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){avail=r.w-t;p=x-r.x-t/2;}else{avail=r.h-t;p=y-r.y-t/2;}
    if(avail<=0)return ctl->spec.range_min;if(p<0)p=0;if(p>avail)p=avail;
    return ctl->spec.range_min+(uint32_t)(((uint64_t)p*(uint64_t)span+(uint64_t)avail/2u)/(uint64_t)avail);
}

static void host_ui_draw_splitter(SDL_Renderer *ren,int idx,HostUiControl *ctl){
    SDL_Rect bar;NyotaColor c=ctl->spec.sep_color;
    if(!host_ui_splitter_bar_rect(idx,&bar))return;
    if(ctl->hover||ctl->range_dragging)c=host_ui_tint_color(c,28);
    SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);host_ui_fill_rect_masked(ren,idx,bar);
}

static NyotaColor host_ui_bg_sample(const NyotaUiBackground *bg,double t){return host_ui_eq_sample(bg,t);}

static void host_ui_scale_line_endpoints(const HostUiControl *ctl,SDL_Rect r,int *x0,int *y0,int *x1,int *y1){
    int pad=(int)ctl->spec.scale_thumb_size/2+(int)ctl->spec.track_blur+4;
    if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){*x0=*x1=r.x+r.w/2;*y0=r.y+r.h-pad;*y1=r.y+pad;}
    else{*y0=*y1=r.y+r.h/2;*x0=r.x+pad;*x1=r.x+r.w-pad;}
}

static void host_ui_scale_point(const HostUiControl *ctl,SDL_Rect r,double t,int *x,int *y,double *angle){
    if(t<0.0)t=0.0;if(t>1.0)t=1.0;
    if(ctl->spec.scale_shape==NYOTA_UI_SCALE_LINE){
        int x0,y0,x1,y1;host_ui_scale_line_endpoints(ctl,r,&x0,&y0,&x1,&y1);
        *x=(int)lround((double)x0+((double)x1-x0)*t);*y=(int)lround((double)y0+((double)y1-y0)*t);
        if(angle)*angle=ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL?0.0:90.0;
    }else{
        int cx=r.x+r.w/2,cy=r.y+r.h/2;double rad=(double)ctl->spec.scale_radius,maxr=(double)(r.w<r.h?r.w:r.h)/2.0-4.0;
        double a;
        if(rad>maxr)rad=maxr;if(rad<1.0)rad=1.0;
        a=ctl->spec.scale_shape==NYOTA_UI_SCALE_CIRCLE?(double)ctl->spec.start_angle+360.0*t:(double)ctl->spec.start_angle+((double)ctl->spec.end_angle-(double)ctl->spec.start_angle)*t;
        host_ui_polar_point(cx,cy,rad,a,x,y);if(angle)*angle=a;
    }
}

static void host_ui_draw_track_segment(SDL_Renderer *ren,int idx,HostUiControl *ctl,int x0,int y0,int x1,int y1,double t){
    NyotaColor c=host_ui_bg_sample(&ctl->spec.track_fill,t),g=host_ui_bg_sample(&ctl->spec.track_glow,t);
    if(ctl->spec.track_blur)host_ui_draw_glow_line(ren,idx,x0,y0,x1,y1,g,(int)ctl->spec.track_width,ctl->spec.track_blur);
    host_ui_draw_thick_line(ren,idx,x0,y0,x1,y1,c,(int)ctl->spec.track_width);
}

static void host_ui_draw_scale_track(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    if(ctl->spec.scale_shape==NYOTA_UI_SCALE_LINE&&ctl->spec.track_style==NYOTA_UI_TRACK_SOLID){
        int x0,y0,x1,y1;host_ui_scale_line_endpoints(ctl,r,&x0,&y0,&x1,&y1);host_ui_draw_track_segment(ren,idx,ctl,x0,y0,x1,y1,0.5);return;
    }
    {
        int steps=ctl->spec.scale_shape==NYOTA_UI_SCALE_LINE?80:180,i,px=0,py=0,x,y;double t;
        for(i=0;i<=steps;i++){
            int draw=1;t=(double)i/(double)steps;host_ui_scale_point(ctl,r,t,&x,&y,NULL);
            if(i){
                if(ctl->spec.track_style==NYOTA_UI_TRACK_DASH||ctl->spec.track_style==NYOTA_UI_TRACK_SEGMENT){
                    int on=ctl->spec.track_style==NYOTA_UI_TRACK_SEGMENT?4:8;
                    int off=(int)ctl->spec.track_gap;if(off<1)off=1;
                    draw=(i%(on+off))<on;
                }else if(ctl->spec.track_style==NYOTA_UI_TRACK_DOT){
                    int every=(int)ctl->spec.track_gap+4;if(every<4)every=4;draw=(i%every)==0;
                }
                if(draw){
                    if(ctl->spec.track_style==NYOTA_UI_TRACK_DOT){NyotaColor cc=host_ui_bg_sample(&ctl->spec.track_fill,t);SDL_Rect q={x-(int)ctl->spec.track_width/2,y-(int)ctl->spec.track_width/2,(int)ctl->spec.track_width,(int)ctl->spec.track_width};SDL_SetRenderDrawColor(ren,cc.r,cc.g,cc.b,cc.a);host_ui_fill_circle(ren,idx,q);}
                    else host_ui_draw_track_segment(ren,idx,ctl,px,py,x,y,t);
                }
            }
            px=x;py=y;
        }
    }
}

static void host_ui_draw_tick_mark(SDL_Renderer *ren,int idx,uint8_t style,int xo,int yo,int xi,int yi,NyotaColor c,int width,uint32_t blur){
    double dx=(double)xi-xo,dy=(double)yi-yo,len=sqrt(dx*dx+dy*dy),nx,ny;
    int sz=width*2;if(sz<4)sz=4;if(sz>12)sz=12;
    if(len<1.0)len=1.0;nx=-dy/len;ny=dx/len;
    if(style==NYOTA_UI_TICK_LINE||style==NYOTA_UI_TICK_RECT){
        if(blur)host_ui_draw_glow_line(ren,idx,xo,yo,xi,yi,c,width,blur);
        host_ui_draw_thick_line(ren,idx,xo,yo,xi,yi,c,width);
    }else if(style==NYOTA_UI_TICK_ROUND){
        SDL_Rect q0={xo-sz/2,yo-sz/2,sz,sz},q1={xi-sz/2,yi-sz/2,sz,sz};
        if(blur){NyotaColor g=c;g.a=(uint8_t)(c.a/3);SDL_Rect gg={q0.x-(int)blur,q0.y-(int)blur,q0.w+2*(int)blur,q0.h+2*(int)blur};SDL_SetRenderDrawColor(ren,g.r,g.g,g.b,g.a);host_ui_fill_circle(ren,idx,gg);}
        SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);host_ui_fill_circle(ren,idx,q0);host_ui_fill_circle(ren,idx,q1);
        host_ui_draw_thick_line(ren,idx,xo,yo,xi,yi,c,width);
    }else if(style==NYOTA_UI_TICK_DOT){
        int mx=(xo+xi)/2,my=(yo+yi)/2;SDL_Rect q={mx-sz/2,my-sz/2,sz,sz};
        if(blur){NyotaColor g=c;g.a=(uint8_t)(c.a/3);SDL_Rect gg={q.x-(int)blur,q.y-(int)blur,q.w+2*(int)blur,q.h+2*(int)blur};SDL_SetRenderDrawColor(ren,g.r,g.g,g.b,g.a);host_ui_fill_circle(ren,idx,gg);}
        SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);host_ui_fill_circle(ren,idx,q);
    }else if(style==NYOTA_UI_TICK_TRIANGLE){
        SDL_Point p[3]={{xo,yo},{xi+(int)lround(nx*sz),yi+(int)lround(ny*sz)},{xi-(int)lround(nx*sz),yi-(int)lround(ny*sz)}};
        if(blur)host_ui_draw_glow_line(ren,idx,xo,yo,xi,yi,c,width,blur);
        host_ui_fill_polygon(ren,idx,p,3,c);
    }else{
        int mx=(xo+xi)/2,my=(yo+yi)/2;
        SDL_Point p[4]={{xo,yo},{mx+(int)lround(nx*sz),my+(int)lround(ny*sz)},{xi,yi},{mx-(int)lround(nx*sz),my-(int)lround(ny*sz)}};
        if(blur)host_ui_draw_glow_line(ren,idx,xo,yo,xi,yi,c,width,blur);
        host_ui_fill_polygon(ren,idx,p,4,c);
    }
}

static void host_ui_draw_control_label(SDL_Renderer *ren,const HostUiControl *ctl,const char *text,SDL_Rect r){
    NyotaUiControlSpec ts;
    if(!ren||!ctl||!text)return;
    ts=ctl->spec;
    strncpy(ts.text,text,sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';
    ts.halign=NYOTA_UI_ALIGN_CENTER;ts.valign=NYOTA_UI_VALIGN_MIDDLE;ts.pad_x=0;ts.pad_y=0;
    host_ui_draw_text(ren,&ts,r);
}

static void host_ui_draw_scale_ticks(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    int64_t span=(int64_t)ctl->spec.signed_max-(int64_t)ctl->spec.signed_min,step=ctl->spec.minor_step,v,count=0;
    if(step<=0||span<=0)return;
    if(span/step>512)step=(span+511)/512;
    for(v=ctl->spec.signed_min;v<=ctl->spec.signed_max&&count++<520;v+=step){
        double t=host_ui_value_t((int32_t)v,ctl->spec.signed_min,ctl->spec.signed_max);int x,y,x2,y2;double a=0.0;
        int major=ctl->spec.major_step>0&&((v-ctl->spec.signed_min)%ctl->spec.major_step)==0;
        int len=major?(int)ctl->spec.tick_len:(int)ctl->spec.minor_tick_len;NyotaColor cc=major?ctl->spec.tick_color:ctl->spec.minor_tick_color;
        host_ui_scale_point(ctl,r,t,&x,&y,&a);
        if(ctl->spec.scale_shape==NYOTA_UI_SCALE_LINE){
            if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){x2=x-len;y2=y;}else{x2=x;y2=y+len;}
        }else{
            int cx=r.x+r.w/2,cy=r.y+r.h/2;double dx=(double)x-cx,dy=(double)y-cy,dl=sqrt(dx*dx+dy*dy);if(dl<1.0)dl=1.0;
            x2=x-(int)lround(dx/dl*len);y2=y-(int)lround(dy/dl*len);
        }
        host_ui_draw_tick_mark(ren,idx,ctl->spec.tick_style,x,y,x2,y2,cc,(int)ctl->spec.tick_width,ctl->spec.tick_blur);
        if(major&&ctl->spec.show_labels&&ctl->spec.label_step>0&&((v-ctl->spec.signed_min)%ctl->spec.label_step)==0){
            char buf[32];SDL_Rect lr={x2-28,y2-10,56,20};snprintf(buf,sizeof(buf),"%lld",(long long)v);
            if(ctl->spec.scale_shape==NYOTA_UI_SCALE_LINE){
                if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL){lr.x=x2-58;lr.y=y2-10;}
                else{lr.x=x2-28;lr.y=y2+(int)ctl->spec.label_offset;}
            }else{
                int cx=r.x+r.w/2,cy=r.y+r.h/2;double dx=(double)x-cx,dy=(double)y-cy,dl=sqrt(dx*dx+dy*dy);if(dl<1.0)dl=1.0;
                lr.x=x2-(int)lround(dx/dl*ctl->spec.label_offset)-28;lr.y=y2-(int)lround(dy/dl*ctl->spec.label_offset)-10;
            }
            host_ui_draw_control_label(ren,ctl,buf,lr);
        }
        if(v+step<v)break;
    }
}

static void host_ui_draw_scale_thumb(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    double t=host_ui_value_t(ctl->spec.signed_value,ctl->spec.signed_min,ctl->spec.signed_max),a=0.0;int x,y,sz=(int)ctl->spec.scale_thumb_size,w=(int)ctl->spec.scale_thumb_width;
    NyotaColor c=host_ui_bg_sample(&ctl->spec.scale_thumb_fill,t),g=host_ui_bg_sample(&ctl->spec.scale_thumb_glow,t);
    SDL_Rect q;
    host_ui_scale_point(ctl,r,t,&x,&y,&a);if(sz<2)sz=2;if(w<1)w=1;
    if(ctl->spec.scale_thumb_blur){
        SDL_Rect glow={x-sz/2-(int)ctl->spec.scale_thumb_blur,y-sz/2-(int)ctl->spec.scale_thumb_blur,sz+2*(int)ctl->spec.scale_thumb_blur,sz+2*(int)ctl->spec.scale_thumb_blur};
        NyotaColor gg=g;gg.a=(uint8_t)(g.a/3);SDL_SetRenderDrawColor(ren,gg.r,gg.g,gg.b,gg.a);host_ui_fill_circle(ren,idx,glow);
    }
    q.x=x-sz/2;q.y=y-sz/2;q.w=sz;q.h=sz;
    SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);
    if(ctl->spec.scale_thumb_shape==NYOTA_UI_SCALE_THUMB_RECT)host_ui_fill_rect_masked(ren,idx,q);
    else if(ctl->spec.scale_thumb_shape==NYOTA_UI_SCALE_THUMB_ROUND)host_ui_fill_round_rect(ren,idx,q);
    else if(ctl->spec.scale_thumb_shape==NYOTA_UI_SCALE_THUMB_CIRCLE||ctl->spec.scale_thumb_shape==NYOTA_UI_SCALE_THUMB_DOT)host_ui_fill_circle(ren,idx,q);
    else if(ctl->spec.scale_thumb_shape==NYOTA_UI_SCALE_THUMB_DIAMOND)host_ui_fill_diamond(ren,idx,q);
    else if(ctl->spec.scale_thumb_shape==NYOTA_UI_SCALE_THUMB_LINE||ctl->spec.scale_thumb_shape==NYOTA_UI_SCALE_THUMB_NEEDLE){
        double da=ctl->spec.thumb_align==NYOTA_UI_THUMB_ALIGN_TANGENT?a+90.0:(ctl->spec.thumb_align==NYOTA_UI_THUMB_ALIGN_FIXED?0.0:a);
        SDL_Point p1=host_ui_rot_point(x,y,-sz/2.0,0,da),p2=host_ui_rot_point(x,y,sz/2.0,0,da);
        host_ui_draw_thick_line(ren,idx,p1.x,p1.y,p2.x,p2.y,c,w);
    }else{
        double da=ctl->spec.thumb_rotate?(ctl->spec.thumb_align==NYOTA_UI_THUMB_ALIGN_TANGENT?a+90.0:(ctl->spec.thumb_align==NYOTA_UI_THUMB_ALIGN_FIXED?0.0:a)):0.0;
        SDL_Point p[4];
        if(ctl->spec.scale_thumb_shape==NYOTA_UI_SCALE_THUMB_TRIANGLE){p[0]=host_ui_rot_point(x,y,sz/2.0,0,da);p[1]=host_ui_rot_point(x,y,-sz/2.0,-sz/2.0,da);p[2]=host_ui_rot_point(x,y,-sz/2.0,sz/2.0,da);host_ui_fill_polygon(ren,idx,p,3,c);}
        else{p[0]=host_ui_rot_point(x,y,0,-sz/2.0,da);p[1]=host_ui_rot_point(x,y,sz/2.0,0,da);p[2]=host_ui_rot_point(x,y,0,sz/2.0,da);p[3]=host_ui_rot_point(x,y,-sz/2.0,0,da);host_ui_fill_polygon(ren,idx,p,4,c);}
    }
    if(ctl->spec.scale_thumb_border&&ctl->spec.scale_thumb_border_color.mode!=NYOTA_COLOR_TRANSPARENT){
        NyotaColor bc=ctl->spec.scale_thumb_border_color;SDL_SetRenderDrawColor(ren,bc.r,bc.g,bc.b,bc.a);SDL_RenderDrawRect(ren,&q);
    }
}

static int32_t host_ui_scale_value_from_pointer(int idx,int mx,int my){
    HostUiControl *ctl=&g_host_ui_controls[idx];SDL_Rect r;double t=0.0;int64_t span,v,step;
    if(!host_ui_control_rect_index(idx,&r)||ctl->spec.signed_max<=ctl->spec.signed_min)return ctl->spec.signed_value;
    if(ctl->spec.scale_shape==NYOTA_UI_SCALE_LINE){
        int x0,y0,x1,y1;host_ui_scale_line_endpoints(ctl,r,&x0,&y0,&x1,&y1);
        if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL)t=(double)(y0-my)/(double)(y0-y1?y0-y1:1);
        else t=(double)(mx-x0)/(double)(x1-x0?x1-x0:1);
    }else{
        int cx=r.x+r.w/2,cy=r.y+r.h/2;double a=host_ui_norm_deg(atan2((double)mx-cx,-((double)my-cy))*180.0/3.14159265358979323846);
        double start=host_ui_norm_deg((double)ctl->spec.start_angle);
        if(ctl->spec.scale_shape==NYOTA_UI_SCALE_CIRCLE){
            double d=host_ui_norm_deg(a-start);t=d/360.0;
        }else{
            double rawspan=(double)ctl->spec.end_angle-(double)ctl->spec.start_angle,dir=rawspan>=0.0?1.0:-1.0,aspan=fabs(rawspan),d;
            if(aspan>360.0)aspan=360.0;
            d=dir>0.0?host_ui_norm_deg(a-start):host_ui_norm_deg(start-a);
            if(d>aspan){double ds=fabs(host_ui_norm_deg(a-start)),de=fabs(host_ui_norm_deg(a-host_ui_norm_deg((double)ctl->spec.end_angle)));t=ds<de?0.0:1.0;}
            else t=d/(aspan>0.0?aspan:1.0);
        }
    }
    if(t<0.0)t=0.0;if(t>1.0)t=1.0;span=(int64_t)ctl->spec.signed_max-(int64_t)ctl->spec.signed_min;
    v=(int64_t)ctl->spec.signed_min+(int64_t)llround((double)span*t);step=ctl->spec.signed_step>0?ctl->spec.signed_step:1;
    v=(int64_t)ctl->spec.signed_min+llround((double)(v-ctl->spec.signed_min)/(double)step)*step;
    if(ctl->spec.scale_wrap&&v>=ctl->spec.signed_max)v=ctl->spec.signed_min;
    if(v<ctl->spec.signed_min)v=ctl->spec.signed_min;if(!ctl->spec.scale_wrap&&v>ctl->spec.signed_max)v=ctl->spec.signed_max;
    return (int32_t)v;
}

static void host_ui_draw_scale(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    host_ui_draw_scale_track(ren,idx,ctl,r);host_ui_draw_scale_ticks(ren,idx,ctl,r);host_ui_draw_scale_thumb(ren,idx,ctl,r);
}

static void host_ui_draw_arc_color(SDL_Renderer *ren,int idx,int cx,int cy,double radius,double a0,double a1,NyotaColor c,int width,uint32_t blur){
    int steps=(int)(fabs(a1-a0)*1.2);int i,px=0,py=0,x,y;if(steps<8)steps=8;if(steps>720)steps=720;
    for(i=0;i<=steps;i++){double t=(double)i/(double)steps,a=a0+(a1-a0)*t;host_ui_polar_point(cx,cy,radius,a,&x,&y);if(i){if(blur)host_ui_draw_glow_line(ren,idx,px,py,x,y,c,width,blur);host_ui_draw_thick_line(ren,idx,px,py,x,y,c,width);}px=x;py=y;}
}

static void host_ui_draw_clock_needle(SDL_Renderer *ren,int idx,int cx,int cy,double a,uint8_t shape,NyotaColor c,int width,int len,uint32_t blur){
    SDL_Point p[5],tip,back,l1,l2;double hw=width>1?width*0.9:2.0;
    tip=host_ui_rot_point(cx,cy,(double)len,0,a);back=host_ui_rot_point(cx,cy,-(double)len*0.12,0,a);
    if(blur)host_ui_draw_glow_line(ren,idx,cx,cy,tip.x,tip.y,c,width,blur);
    if(shape==NYOTA_UI_NEEDLE_LINE){host_ui_draw_thick_line(ren,idx,cx,cy,tip.x,tip.y,c,width);return;}
    if(shape==NYOTA_UI_NEEDLE_DOUBLE){SDL_Point b=host_ui_rot_point(cx,cy,-(double)len*0.45,0,a);host_ui_draw_thick_line(ren,idx,b.x,b.y,tip.x,tip.y,c,width);return;}
    if(shape==NYOTA_UI_NEEDLE_BAR){host_ui_draw_thick_line(ren,idx,back.x,back.y,tip.x,tip.y,c,width*2);return;}
    if(shape==NYOTA_UI_NEEDLE_TRIANGLE){
        p[0]=tip;p[1]=host_ui_rot_point(cx,cy,-len*0.08,-hw,a);p[2]=host_ui_rot_point(cx,cy,-len*0.08,hw,a);host_ui_fill_polygon(ren,idx,p,3,c);return;
    }
    if(shape==NYOTA_UI_NEEDLE_ARROW){
        SDL_Point shaft=host_ui_rot_point(cx,cy,len*0.72,0,a);host_ui_draw_thick_line(ren,idx,cx,cy,shaft.x,shaft.y,c,width);
        p[0]=tip;p[1]=host_ui_rot_point(cx,cy,len*0.68,-hw*2.0,a);p[2]=host_ui_rot_point(cx,cy,len*0.68,hw*2.0,a);host_ui_fill_polygon(ren,idx,p,3,c);return;
    }
    if(shape==NYOTA_UI_NEEDLE_DIAMOND){
        p[0]=tip;p[1]=host_ui_rot_point(cx,cy,len*0.42,-hw*1.5,a);p[2]=back;p[3]=host_ui_rot_point(cx,cy,len*0.42,hw*1.5,a);host_ui_fill_polygon(ren,idx,p,4,c);return;
    }
    l1=host_ui_rot_point(cx,cy,-len*0.08,-hw,a);l2=host_ui_rot_point(cx,cy,-len*0.08,hw,a);
    p[0]=l1;p[1]=host_ui_rot_point(cx,cy,len*0.92,-hw,a);p[2]=tip;p[3]=host_ui_rot_point(cx,cy,len*0.84,hw,a);p[4]=l2;host_ui_fill_polygon(ren,idx,p,5,c);
}

static void host_ui_draw_clock(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    int cx=r.x+r.w/2,cy=r.y+r.h/2;double radius=(double)ctl->spec.clock_radius,maxr=(double)(r.w<r.h?r.w:r.h)/2.0-6.0;
    double a0=(double)ctl->spec.start_angle,a1=ctl->spec.clock_shape==NYOTA_UI_CLOCK_CIRCLE?a0+360.0:(double)ctl->spec.end_angle;
    uint32_t i;SDL_Rect dial;int d;
    if(radius>maxr)radius=maxr;if(radius<8.0)radius=8.0;d=(int)(radius*2.0);dial.x=cx-d/2;dial.y=cy-d/2;dial.w=d;dial.h=d;
    {
        NyotaUiControlSpec ts=ctl->spec;SDL_Surface *sf;SDL_Texture *tx;ts.kind=NYOTA_UI_CTRL_RADIO;ts.w=(uint32_t)d;ts.h=(uint32_t)d;ts.radius=0;
        sf=host_ui_control_background_surface(&ts,&ctl->spec.background);
        if(sf){host_ui_apply_ancestor_mask(idx,sf,dial.x,dial.y);tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);if(tx){SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);SDL_RenderCopy(ren,tx,NULL,&dial);SDL_DestroyTexture(tx);}}
    }
    if(ctl->spec.dial_blur&&ctl->spec.border_color.mode!=NYOTA_COLOR_TRANSPARENT)host_ui_draw_arc_color(ren,idx,cx,cy,radius,a0,a1,ctl->spec.border_color,(int)(ctl->spec.border_width?ctl->spec.border_width:1),ctl->spec.dial_blur);
    if(ctl->spec.border&&ctl->spec.border_color.mode!=NYOTA_COLOR_TRANSPARENT)host_ui_draw_arc_color(ren,idx,cx,cy,radius,a0,a1,ctl->spec.border_color,(int)(ctl->spec.border_width?ctl->spec.border_width:1),0);

    for(i=0;i<ctl->spec.zone_count;i++){
        double z0=host_ui_value_t(ctl->spec.zone_min[i],ctl->spec.signed_min,ctl->spec.signed_max);
        double z1=host_ui_value_t(ctl->spec.zone_max[i],ctl->spec.signed_min,ctl->spec.signed_max);
        host_ui_draw_arc_color(ren,idx,cx,cy,radius-5.0,a0+(a1-a0)*z0,a0+(a1-a0)*z1,ctl->spec.zone_color[i],5,1);
    }
    {
        int64_t span=(int64_t)ctl->spec.signed_max-(int64_t)ctl->spec.signed_min,step=ctl->spec.minor_step,v,count=0;
        if(step<=0)step=1;if(span/step>360)step=(span+359)/360;
        for(v=ctl->spec.signed_min;v<=ctl->spec.signed_max&&count++<370;v+=step){
            double t=host_ui_value_t((int32_t)v,ctl->spec.signed_min,ctl->spec.signed_max),a=a0+(a1-a0)*t;int xo,yo,xi,yi;
            int major=ctl->spec.major_step>0&&((v-ctl->spec.signed_min)%ctl->spec.major_step)==0;
            int tl=major?(int)ctl->spec.tick_len:(int)ctl->spec.minor_tick_len;NyotaColor cc=major?ctl->spec.tick_color:ctl->spec.minor_tick_color;
            host_ui_polar_point(cx,cy,radius-8.0,a,&xo,&yo);host_ui_polar_point(cx,cy,radius-8.0-tl,a,&xi,&yi);
            host_ui_draw_tick_mark(ren,idx,ctl->spec.tick_style,xo,yo,xi,yi,cc,(int)ctl->spec.tick_width,ctl->spec.tick_blur);
            if(major&&ctl->spec.show_labels&&ctl->spec.label_step>0&&((v-ctl->spec.signed_min)%ctl->spec.label_step)==0){
                int lx,ly;char buf[32];SDL_Rect lr;host_ui_polar_point(cx,cy,radius-8.0-tl-(double)ctl->spec.label_offset,a,&lx,&ly);
                snprintf(buf,sizeof(buf),"%lld",(long long)v);lr.x=lx-28;lr.y=ly-10;lr.w=56;lr.h=20;host_ui_draw_control_label(ren,ctl,buf,lr);
            }
            if(v+step<v)break;
        }
    }
    for(i=0;i<ctl->spec.clock_needles;i++){
        int32_t minv=ctl->spec.clock_needle_min[i],maxv=ctl->spec.clock_needle_max[i],val=i<ctl->spec.clock_value_count?ctl->spec.clock_values[i]:minv;
        double t=host_ui_value_t(val,minv,maxv),a=a0+(a1-a0)*t;int len=(int)ctl->spec.clock_needle_len[i];if(len<=0||len>(int)radius-12)len=(int)radius-12;
        host_ui_draw_clock_needle(ren,idx,cx,cy,a,ctl->spec.clock_needle_shape[i],ctl->spec.clock_needle_color[i],(int)ctl->spec.clock_needle_width[i],len,ctl->spec.clock_needle_blur[i]);
    }
    if(ctl->spec.center_dot){
        int sz=(int)ctl->spec.center_size;SDL_Rect q={cx-sz/2,cy-sz/2,sz,sz};NyotaColor cc=ctl->spec.center_color;
        if(ctl->spec.center_blur){SDL_Rect g={q.x-(int)ctl->spec.center_blur,q.y-(int)ctl->spec.center_blur,q.w+2*(int)ctl->spec.center_blur,q.h+2*(int)ctl->spec.center_blur};NyotaColor gc=cc;gc.a=(uint8_t)(cc.a/3);SDL_SetRenderDrawColor(ren,gc.r,gc.g,gc.b,gc.a);host_ui_fill_circle(ren,idx,g);}
        SDL_SetRenderDrawColor(ren,cc.r,cc.g,cc.b,cc.a);host_ui_fill_circle(ren,idx,q);
    }
    if(ctl->spec.text[0]){SDL_Rect tr={r.x,r.y+(int)(r.h*0.12),r.w,28};NyotaUiControlSpec ts=ctl->spec;ts.halign=NYOTA_UI_ALIGN_CENTER;ts.valign=NYOTA_UI_VALIGN_MIDDLE;host_ui_draw_text(ren,&ts,tr);}
    if(ctl->spec.show_value&&ctl->spec.clock_needles){
        char buf[96];SDL_Rect vr={r.x,r.y+(int)(r.h*0.66),r.w,32};NyotaUiControlSpec ts=ctl->spec;
        snprintf(buf,sizeof(buf),"%d%s%s",ctl->spec.clock_values[0],ctl->spec.unit[0]?" ":"",ctl->spec.unit);
        strncpy(ts.text,buf,sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';ts.halign=NYOTA_UI_ALIGN_CENTER;ts.bold=1;host_ui_draw_text(ren,&ts,vr);
    }
}


static void host_ui_draw_background_shape(SDL_Renderer *ren,int idx,SDL_Rect r,const NyotaUiBackground *bg,uint32_t radius){
    NyotaUiControlSpec ts;SDL_Surface *sf;SDL_Texture *tx;
    if(!ren||!bg||r.w<=0||r.h<=0)return;
    memset(&ts,0,sizeof(ts));ts.kind=NYOTA_UI_CTRL_PANEL;ts.w=(uint32_t)r.w;ts.h=(uint32_t)r.h;ts.radius=radius;ts.background=*bg;
    sf=host_ui_control_background_surface(&ts,bg);if(!sf)return;
    host_ui_apply_ancestor_mask(idx,sf,r.x,r.y);
    tx=SDL_CreateTextureFromSurface(ren,sf);SDL_FreeSurface(sf);
    if(tx){SDL_SetTextureBlendMode(tx,SDL_BLENDMODE_BLEND);SDL_RenderCopy(ren,tx,NULL,&r);SDL_DestroyTexture(tx);}
}

static void host_ui_draw_iconbutton(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    SDL_Texture *it=NULL;SDL_Rect content=r,ir={0,0,0,0},tr=r;int sz=(int)ctl->spec.icon_size,gap=(int)ctl->spec.icon_gap,tw=0,th=0;
    NyotaUiControlSpec ts=ctl->spec;uint8_t alpha=ctl->spec.enabled?255:120;
    content.x+=(int)ctl->spec.pad_x;content.y+=(int)ctl->spec.pad_y;content.w-=2*(int)ctl->spec.pad_x;content.h-=2*(int)ctl->spec.pad_y;
    if(content.w<=0||content.h<=0)return;
    if(sz<4)sz=4;if(sz>content.w)sz=content.w;if(sz>content.h)sz=content.h;
    if(ctl->spec.icon_path[0])it=host_ui_texture_from_path(ren,&ctl->icon_cache,ctl->spec.icon_path);
    if(!ctl->spec.text[0]&&it){ir=content;host_ui_draw_texture_fit(ren,it,ir,alpha);return;}
    if(it){
        if(ctl->spec.icon_pos==NYOTA_UI_ICON_RIGHT){
            ir=(SDL_Rect){content.x+content.w-sz,content.y+(content.h-sz)/2,sz,sz};
            tr=(SDL_Rect){content.x,content.y,content.w-sz-gap,content.h};
        }else if(ctl->spec.icon_pos==NYOTA_UI_ICON_TOP){
            ir=(SDL_Rect){content.x+(content.w-sz)/2,content.y,sz,sz};
            tr=(SDL_Rect){content.x,content.y+sz+gap,content.w,content.h-sz-gap};
        }else if(ctl->spec.icon_pos==NYOTA_UI_ICON_BOTTOM){
            ir=(SDL_Rect){content.x+(content.w-sz)/2,content.y+content.h-sz,sz,sz};
            tr=(SDL_Rect){content.x,content.y,content.w,content.h-sz-gap};
        }else if(ctl->spec.icon_pos==NYOTA_UI_ICON_CENTER){
            ir=(SDL_Rect){content.x+(content.w-sz)/2,content.y+(content.h-sz)/2,sz,sz};
            tr=content;
        }else{
            ir=(SDL_Rect){content.x,content.y+(content.h-sz)/2,sz,sz};
            tr=(SDL_Rect){content.x+sz+gap,content.y,content.w-sz-gap,content.h};
        }
        host_ui_draw_texture_fit(ren,it,ir,alpha);
    }
    if(ctl->spec.text[0]){
        ts.w=(uint32_t)(tr.w>0?tr.w:1);ts.h=(uint32_t)(tr.h>0?tr.h:1);ts.pad_x=ts.pad_y=0;
        ts.halign=NYOTA_UI_ALIGN_CENTER;ts.valign=NYOTA_UI_VALIGN_MIDDLE;
        if(!ctl->spec.enabled)ts.text_color=host_ui_tint_color(ts.text_color,-72);
        (void)tw;(void)th;host_ui_draw_text(ren,&ts,tr);
    }
}

static void host_ui_draw_switch(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    int th=r.h<36?r.h:36,tw=r.w;int sz=(int)ctl->spec.switch_thumb_size;SDL_Rect track,thumb,glow;const NyotaUiBackground *tb;
    NyotaUiControlSpec ts=ctl->spec;char *label;
    if(th<16)th=16;if(tw<2*th)tw=2*th;if(tw>r.w)tw=r.w;
    track.x=r.x;track.y=r.y+(r.h-th)/2;track.w=tw;track.h=th;
    tb=ctl->spec.checked?&ctl->spec.switch_track_on:&ctl->spec.switch_track_off;
    if(ctl->spec.switch_blur&&ctl->spec.checked){
        int b=(int)ctl->spec.switch_blur;glow=(SDL_Rect){track.x-b,track.y-b,track.w+2*b,track.h+2*b};
        host_ui_draw_background_shape(ren,idx,glow,&ctl->spec.switch_glow,(uint32_t)(glow.h/2));
    }
    host_ui_draw_background_shape(ren,idx,track,tb,(uint32_t)(track.h/2));
    if(sz>track.h-4)sz=track.h-4;if(sz<8)sz=8;
    thumb.w=thumb.h=sz;thumb.y=track.y+(track.h-sz)/2;
    thumb.x=ctl->spec.checked?track.x+track.w-sz-2:track.x+2;
    host_ui_draw_background_shape(ren,idx,thumb,&ctl->spec.switch_thumb_fill,(uint32_t)(sz/2));
    label=ctl->spec.checked?ctl->spec.switch_on_text:ctl->spec.switch_off_text;
    if(label[0]&&track.w>sz+26){
        SDL_Rect lr=track;
        strncpy(ts.text,label,sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';ts.pad_x=ts.pad_y=0;ts.font_size=ts.font_size>11?11:ts.font_size;
        if(ctl->spec.checked){lr.x=track.x+4;lr.w=track.w-sz-10;}else{lr.x=track.x+sz+6;lr.w=track.w-sz-10;}
        ts.halign=NYOTA_UI_ALIGN_CENTER;ts.valign=NYOTA_UI_VALIGN_MIDDLE;
        host_ui_draw_text(ren,&ts,lr);
    }
}

static int host_ui_input_edit_rect(int idx,SDL_Rect full,SDL_Rect *edit){
    HostUiControl *ctl;int left=6,right=6,sz;
    if(!edit||idx<0||idx>=HOST_MAX_UI_CONTROLS)return 0;ctl=&g_host_ui_controls[idx];
    if(!ctl->used||ctl->spec.kind!=NYOTA_UI_CTRL_INPUT)return 0;
    sz=(int)ctl->spec.icon_size;if(sz<4)sz=4;if(sz>full.h-4)sz=full.h-4;
    if(ctl->spec.icon_path[0]){
        if(ctl->spec.icon_pos==NYOTA_UI_ICON_RIGHT)right+=sz+(int)ctl->spec.icon_gap;
        else left+=sz+(int)ctl->spec.icon_gap;
    }
    if(ctl->spec.prefix[0]){int w=0,h=0;if(!host_ui_measure_text(ctl->spec.font,ctl->spec.prefix,ctl->spec.font_size,ctl->spec.bold,ctl->spec.italic,ctl->spec.underline,&w,&h))w=(int)strlen(ctl->spec.prefix)*(int)ctl->spec.font_size/2;left+=w+5;}
    if(ctl->spec.suffix[0]){int w=0,h=0;if(!host_ui_measure_text(ctl->spec.font,ctl->spec.suffix,ctl->spec.font_size,ctl->spec.bold,ctl->spec.italic,ctl->spec.underline,&w,&h))w=(int)strlen(ctl->spec.suffix)*(int)ctl->spec.font_size/2;right+=w+5;}
    if(ctl->spec.clear_button)right+=full.h;
    *edit=full;edit->x+=left;edit->w-=left+right;if(edit->w<8)edit->w=8;return 1;
}

static void host_ui_draw_input(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    SDL_Rect er,ir,pr,sr,cr;SDL_Texture *it=NULL;int sz=(int)ctl->spec.icon_size;NyotaUiControlSpec ts=ctl->spec;
    if(!host_ui_input_edit_rect(idx,r,&er))return;
    if(sz>r.h-6)sz=r.h-6;if(sz<4)sz=4;
    if(ctl->spec.icon_path[0]){
        it=host_ui_texture_from_path(ren,&ctl->icon_cache,ctl->spec.icon_path);
        if(ctl->spec.icon_pos==NYOTA_UI_ICON_RIGHT)ir=(SDL_Rect){r.x+r.w-r.h+(r.h-sz)/2,r.y+(r.h-sz)/2,sz,sz};
        else ir=(SDL_Rect){r.x+5,r.y+(r.h-sz)/2,sz,sz};
        if(it)host_ui_draw_texture_fit(ren,it,ir,ctl->spec.enabled?255:120);
    }
    if(ctl->spec.prefix[0]){
        pr=(SDL_Rect){r.x+(ctl->spec.icon_path[0]&&ctl->spec.icon_pos!=NYOTA_UI_ICON_RIGHT?sz+(int)ctl->spec.icon_gap+6:6),r.y,er.x-r.x-6,r.h};
        strncpy(ts.text,ctl->spec.prefix,sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';ts.pad_x=ts.pad_y=0;ts.halign=NYOTA_UI_ALIGN_LEFT;ts.valign=NYOTA_UI_VALIGN_MIDDLE;host_ui_draw_text(ren,&ts,pr);
    }
    if(ctl->spec.suffix[0]){
        int sw=0,sh=0;host_ui_measure_text(ctl->spec.font,ctl->spec.suffix,ctl->spec.font_size,ctl->spec.bold,ctl->spec.italic,ctl->spec.underline,&sw,&sh);
        sr=(SDL_Rect){er.x+er.w+4,r.y,sw+4,r.h};
        strncpy(ts.text,ctl->spec.suffix,sizeof(ts.text)-1);ts.text[sizeof(ts.text)-1]='\0';ts.pad_x=ts.pad_y=0;ts.halign=NYOTA_UI_ALIGN_LEFT;ts.valign=NYOTA_UI_VALIGN_MIDDLE;host_ui_draw_text(ren,&ts,sr);
    }
    host_ui_draw_tbox(ren,ctl,er,idx);
    if(ctl->spec.clear_button){
        int d=r.h>28?28:r.h-4;cr=(SDL_Rect){r.x+r.w-r.h+(r.h-d)/2,r.y+(r.h-d)/2,d,d};
        if(ctl->spec.text[0]){
            NyotaColor cc=ctl->spec.text_color;SDL_SetRenderDrawColor(ren,cc.r,cc.g,cc.b,cc.a);
            host_ui_draw_line_masked(ren,idx,cr.x+7,cr.y+7,cr.x+cr.w-8,cr.y+cr.h-8);
            host_ui_draw_line_masked(ren,idx,cr.x+cr.w-8,cr.y+7,cr.x+7,cr.y+cr.h-8);
        }
    }
}

static void host_ui_draw_frame(SDL_Renderer *ren,int idx,HostUiControl *ctl,SDL_Rect r){
    int tw=0,th=0,pad=(int)ctl->spec.frame_title_pad;SDL_Rect patch,tr;NyotaUiControlSpec ts=ctl->spec;
    if(!ctl->spec.text[0])return;
    if(!host_ui_measure_text(ctl->spec.font,ctl->spec.text,ctl->spec.font_size,ctl->spec.bold,ctl->spec.italic,ctl->spec.underline,&tw,&th)){tw=(int)strlen(ctl->spec.text)*(int)ctl->spec.font_size/2;th=(int)ctl->spec.font_size+2;}
    patch.w=tw+2*pad;patch.h=th+4;patch.y=r.y;
    if(ctl->spec.frame_title_pos==NYOTA_UI_FRAME_TITLE_TOPCENTER)patch.x=r.x+(r.w-patch.w)/2;
    else if(ctl->spec.frame_title_pos==NYOTA_UI_FRAME_TITLE_TOPRIGHT)patch.x=r.x+r.w-patch.w-pad;
    else patch.x=r.x+pad;
    if(patch.x<r.x)patch.x=r.x;if(patch.x+patch.w>r.x+r.w)patch.w=r.x+r.w-patch.x;
    host_ui_draw_bg_rect(ren,idx,patch,&ctl->spec.background);
    tr=patch;ts.pad_x=ts.pad_y=0;ts.halign=NYOTA_UI_ALIGN_CENTER;ts.valign=NYOTA_UI_VALIGN_MIDDLE;
    host_ui_draw_text(ren,&ts,tr);
}

static void host_ui_draw_control_index(int idx) {
    HostUiControl *ctl; HostUiWindow *u; SDL_Rect r,clip; SDL_Surface *surface; SDL_Texture *tex; const NyotaUiBackground *bg; int wi;
    if(idx<0||idx>=HOST_MAX_UI_CONTROLS||!g_host_ui_controls[idx].used)return;ctl=&g_host_ui_controls[idx];wi=host_ui_index_by_handle(ctl->window_handle);
    if(wi<0||!(u=&g_ui_windows[wi])->ren)return;
    if(ctl->spec.kind==NYOTA_UI_CTRL_TAB) host_ui_draw_tab_header(idx,u->ren);
    if(!host_ui_control_visible_index(idx))return;
    if(!host_ui_control_rect_index(idx,&r)||!host_ui_clip_for_control(idx,&clip)||clip.w<=0||clip.h<=0)return;
    SDL_RenderSetClipRect(u->ren,&clip);
    g_ui_draw_clip_idx=idx;
    if(ctl->spec.kind==NYOTA_UI_CTRL_TABS){g_ui_draw_clip_idx=-1;SDL_RenderSetClipRect(u->ren,NULL);return;}
    host_ui_draw_shadow(u->ren,ctl,r);
    if(ctl->spec.kind==NYOTA_UI_CTRL_SEP){
        uint32_t t=ctl->spec.sep_thickness;NyotaColor c=ctl->spec.sep_color;SDL_Rect q=r;
        if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL)q.w=(int)t;else q.h=(int)t;
        if(ctl->spec.sep_effect==NYOTA_UI_SEP_GRADIENT){
            NyotaUiControlSpec ts=ctl->spec;ts.w=(uint32_t)q.w;ts.h=(uint32_t)q.h;surface=host_ui_control_background_surface(&ts,&ctl->spec.background);
            if(surface){tex=SDL_CreateTextureFromSurface(u->ren,surface);SDL_FreeSurface(surface);if(tex){SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);SDL_RenderCopy(u->ren,tex,NULL,&q);SDL_DestroyTexture(tex);}}
        } else {
            SDL_SetRenderDrawColor(u->ren,c.r,c.g,c.b,c.a);host_ui_fill_rect_masked(u->ren,idx,q);
            if(ctl->spec.sep_effect==NYOTA_UI_SEP_INSET||ctl->spec.sep_effect==NYOTA_UI_SEP_RAISED){
                NyotaColor hi=c,lo=c;hi.r=(uint8_t)(hi.r+(255-hi.r)/2);hi.g=(uint8_t)(hi.g+(255-hi.g)/2);hi.b=(uint8_t)(hi.b+(255-hi.b)/2);lo.r/=2;lo.g/=2;lo.b/=2;
                NyotaColor a=ctl->spec.sep_effect==NYOTA_UI_SEP_INSET?lo:hi,b=ctl->spec.sep_effect==NYOTA_UI_SEP_INSET?hi:lo;
                SDL_SetRenderDrawColor(u->ren,a.r,a.g,a.b,a.a);if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL)host_ui_draw_line_masked(u->ren,idx,q.x,q.y,q.x,q.y+q.h-1);else host_ui_draw_line_masked(u->ren,idx,q.x,q.y,q.x+q.w-1,q.y);
                SDL_SetRenderDrawColor(u->ren,b.r,b.g,b.b,b.a);if(ctl->spec.orientation==NYOTA_UI_SEP_VERTICAL)host_ui_draw_line_masked(u->ren,idx,q.x+q.w-1,q.y,q.x+q.w-1,q.y+q.h-1);else host_ui_draw_line_masked(u->ren,idx,q.x,q.y+q.h-1,q.x+q.w-1,q.y+q.h-1);
            }
        }
        g_ui_draw_clip_idx=-1;SDL_RenderSetClipRect(u->ren,NULL);return;
    }
    {
        NyotaUiBackground state_bg;
        bg=&ctl->spec.background;
        if(!ctl->spec.enabled){
            host_ui_tint_background(&ctl->spec.background,&state_bg,-28);
            bg=&state_bg;
        } else if(ctl->spec.kind==NYOTA_UI_CTRL_DAREA&&ctl->hover) bg=&ctl->spec.background_over;
        else if((ctl->spec.kind==NYOTA_UI_CTRL_BUTTON||ctl->spec.kind==NYOTA_UI_CTRL_CBOX||
                 ctl->spec.kind==NYOTA_UI_CTRL_RADIO||ctl->spec.kind==NYOTA_UI_CTRL_COMBO) &&
                (ctl->hover||ctl->pressed)){
            host_ui_tint_background(&ctl->spec.background,&state_bg,ctl->pressed?-18:10);
            bg=&state_bg;
        }
        surface=ctl->spec.kind==NYOTA_UI_CTRL_CLOCK?NULL:host_ui_control_background_surface(&ctl->spec,bg);
    }if(surface){host_ui_apply_ancestor_mask(idx,surface,r.x,r.y);tex=SDL_CreateTextureFromSurface(u->ren,surface);SDL_FreeSurface(surface);if(tex){SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);SDL_RenderCopy(u->ren,tex,NULL,&r);SDL_DestroyTexture(tex);}}
    if(ctl->spec.kind!=NYOTA_UI_CTRL_CLOCK)host_ui_draw_border(u->ren,ctl,r);
    if(ctl->spec.kind==NYOTA_UI_CTRL_CBOX&&ctl->spec.checked){
        if(!strcmp(ctl->spec.check_symbol,"X")){
            int pad=r.w/4;if(pad<3)pad=3;
            NyotaColor cc=ctl->spec.enabled?ctl->spec.check_color:host_ui_tint_color(ctl->spec.check_color,-70);
            SDL_SetRenderDrawColor(u->ren,cc.r,cc.g,cc.b,cc.a);
            host_ui_draw_line_masked(u->ren,idx,r.x+pad,r.y+pad,r.x+r.w-1-pad,r.y+r.h-1-pad);
            host_ui_draw_line_masked(u->ren,idx,r.x+r.w-1-pad,r.y+pad,r.x+pad,r.y+r.h-1-pad);
        } else {
            SDL_Rect tr=r;NyotaColor cc=ctl->spec.enabled?ctl->spec.check_color:host_ui_tint_color(ctl->spec.check_color,-70);host_ui_draw_simple_text(u->ren,ctl->spec.check_symbol,cc,tr,ctl->spec.w>8?ctl->spec.w-4:8);
        }
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_RADIO&&ctl->spec.checked){
        int rr=(r.w<r.h?r.w:r.h)/4;SDL_Rect dot={r.x+r.w/2-rr,r.y+r.h/2-rr,rr*2,rr*2};NyotaColor cc=ctl->spec.enabled?ctl->spec.check_color:host_ui_tint_color(ctl->spec.check_color,-70);SDL_SetRenderDrawColor(u->ren,cc.r,cc.g,cc.b,cc.a);
        for(int yy=-rr;yy<=rr;yy++){int xx=(int)sqrt((double)(rr*rr-yy*yy));host_ui_draw_line_masked(u->ren,idx,r.x+r.w/2-xx,r.y+r.h/2+yy,r.x+r.w/2+xx,r.y+r.h/2+yy);}
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_COMBO){
        char item[NYOTA_UI_TEXT_MAX];SDL_Rect txr=r;txr.w-=r.h;
        {NyotaColor tc=ctl->spec.enabled?ctl->spec.text_color:host_ui_tint_color(ctl->spec.text_color,-72);
        if(host_ui_item_at(ctl->spec.items,ctl->spec.selected,item,sizeof(item)))host_ui_draw_simple_text(u->ren,item,tc,txr,ctl->spec.font_size);}
        {NyotaColor ac=ctl->spec.enabled?ctl->spec.arrow_color:host_ui_tint_color(ctl->spec.arrow_color,-72);
        SDL_SetRenderDrawColor(u->ren,ac.r,ac.g,ac.b,ac.a);}int cx=r.x+r.w-r.h/2,cy=r.y+r.h/2;host_ui_draw_line_masked(u->ren,idx,cx-5,cy-2,cx,cy+3);host_ui_draw_line_masked(u->ren,idx,cx,cy+3,cx+5,cy-2);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_TAREA){
        NyotaUiControlSpec saved=ctl->spec;
        if(!ctl->spec.enabled)ctl->spec.text_color=host_ui_tint_color(ctl->spec.text_color,-72);
        host_ui_draw_tarea(u->ren,ctl,r,idx);
        ctl->spec=saved;
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_TBOX){
        NyotaUiControlSpec saved=ctl->spec;
        if(!ctl->spec.enabled)ctl->spec.text_color=host_ui_tint_color(ctl->spec.text_color,-72);
        host_ui_draw_tbox(u->ren,ctl,r,idx);
        ctl->spec=saved;
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_SPINBOX){
        host_ui_draw_spinbox(u->ren,idx,ctl,r);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_LISTVIEW){
        host_ui_draw_listview(u->ren,idx,ctl,r,0);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_TREEVIEW){
        host_ui_draw_listview(u->ren,idx,ctl,r,1);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_SPLITTER){
        host_ui_draw_splitter(u->ren,idx,ctl);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_SCALE){
        host_ui_draw_scale(u->ren,idx,ctl,r);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_CLOCK){
        host_ui_draw_clock(u->ren,idx,ctl,r);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_SBAR){
        host_ui_draw_sbar_thumb(u->ren,idx,ctl);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_PBAR){
        host_ui_draw_pbar(u->ren,idx,ctl,r);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_SLIDER){
        host_ui_draw_slider_thumb(u->ren,idx,ctl);
    } else if(ctl->spec.kind==NYOTA_UI_CTRL_EQBOX){
        host_ui_draw_eqbox(u->ren,idx,ctl,r);
    } else {
        NyotaUiControlSpec ts=ctl->spec;
        if(!ts.enabled) ts.text_color=host_ui_tint_color(ts.text_color,-72);
        host_ui_draw_text(u->ren,&ts,r);
    }
    host_ui_draw_focus_ring(u->ren,ctl,r);
    g_ui_draw_clip_idx=-1;
    SDL_RenderSetClipRect(u->ren,NULL);
}

static uint32_t host_ui_combo_item_count(const HostUiControl *ctl){
    uint32_t n=0,k;
    if(!ctl)return 0;
    for(k=0;ctl->spec.items[k];k++)if(ctl->spec.items[k]=='\n')n++;
    return n;
}

static int host_ui_combo_popup_hit(int idx,int x,int y,int32_t *row_out){
    HostUiControl *ctl;
    SDL_Rect r;
    uint32_t count,vis;
    int row;
    if(row_out)*row_out=-1;
    if(idx<0||idx>=HOST_MAX_UI_CONTROLS)return 0;
    ctl=&g_host_ui_controls[idx];
    if(!ctl->used||ctl->spec.kind!=NYOTA_UI_CTRL_COMBO||!ctl->combo_open||
       !ctl->spec.enabled||!host_ui_control_visible_index(idx))return 0;
    if(!host_ui_control_rect_index(idx,&r)||r.h<=0)return 0;
    count=host_ui_combo_item_count(ctl);
    vis=count<ctl->spec.max_visible?count:ctl->spec.max_visible;
    if(!vis)return 0;
    if(x<r.x||x>=r.x+r.w||y<r.y+r.h||y>=r.y+r.h+(int)vis*r.h)return 0;
    row=(y-(r.y+r.h))/r.h;
    if(row<0||(uint32_t)row>=vis)return 0;
    if(row_out)*row_out=row;
    return 1;
}

/* Rozwiniete COMBO jest popupem okna: nie dziedziczy Z-order ani clippingu
   rodzica. Rysujemy je po wszystkich zwyklych kontrolkach. */
static void host_ui_draw_combo_popup(int idx){
    HostUiControl *ctl;
    HostUiWindow *u;
    SDL_Rect r,winclip;
    SDL_Surface *surface;
    SDL_Texture *tex;
    uint32_t count,k,vis;
    int wi;
    char item[NYOTA_UI_TEXT_MAX];

    if(idx<0||idx>=HOST_MAX_UI_CONTROLS)return;
    ctl=&g_host_ui_controls[idx];
    if(!ctl->used||ctl->spec.kind!=NYOTA_UI_CTRL_COMBO||!ctl->combo_open||
       !ctl->spec.enabled||!host_ui_control_visible_index(idx))return;
    wi=host_ui_index_by_handle(ctl->window_handle);
    if(wi<0||!(u=&g_ui_windows[wi])->ren||!host_ui_control_rect_index(idx,&r))return;

    count=host_ui_combo_item_count(ctl);
    vis=count<ctl->spec.max_visible?count:ctl->spec.max_visible;
    if(!vis)return;

    winclip.x=0;winclip.y=0;winclip.w=(int)u->w;winclip.h=(int)u->h;
    SDL_RenderSetClipRect(u->ren,&winclip);
    g_ui_draw_clip_idx=-1;

    for(k=0;k<vis;k++){
        SDL_Rect row={r.x,r.y+r.h+(int)k*r.h,r.w,r.h};
        const NyotaUiBackground *rb=&ctl->spec.drop_background;
        NyotaColor tc=ctl->spec.drop_text_color;
        NyotaUiControlSpec rs=ctl->spec;

        if(k==ctl->spec.selected){
            rb=&ctl->spec.selected_background;
            tc=ctl->spec.selected_text_color;
        }
        if(ctl->spec.hover_enabled&&(int32_t)k==ctl->combo_hover){
            rb=&ctl->spec.hover_background;
            tc=ctl->spec.hover_text_color;
        }

        rs.w=(uint32_t)row.w;
        rs.h=(uint32_t)row.h;
        rs.radius=0; /* popup jest jedna warstwa; wiersze nie wycinaja sie nawzajem */
        surface=host_ui_control_background_surface(&rs,rb);
        if(surface){
            tex=SDL_CreateTextureFromSurface(u->ren,surface);
            SDL_FreeSurface(surface);
            if(tex){
                SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);
                SDL_RenderCopy(u->ren,tex,NULL,&row);
                SDL_DestroyTexture(tex);
            }
        }
        if(host_ui_item_at(ctl->spec.items,k,item,sizeof(item)))
            host_ui_draw_simple_text(u->ren,item,tc,row,ctl->spec.font_size);
    }

    /* Obramowanie calego popupu daje wizualne odciecie od kontrolek pod nim. */
    if(ctl->spec.border&&ctl->spec.border_color.mode!=NYOTA_COLOR_TRANSPARENT){
        SDL_Rect pr={r.x,r.y+r.h,r.w,(int)vis*r.h};
        uint32_t bw=ctl->spec.border_width?ctl->spec.border_width:1;
        uint32_t b;
        SDL_SetRenderDrawBlendMode(u->ren,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(u->ren,ctl->spec.border_color.r,ctl->spec.border_color.g,
                               ctl->spec.border_color.b,ctl->spec.border_color.a);
        for(b=0;b<bw;b++){
            SDL_Rect q={pr.x+(int)b,pr.y+(int)b,pr.w-(int)(2*b),pr.h-(int)(2*b)};
            if(q.w<=1||q.h<=1)break;
            SDL_RenderDrawRect(u->ren,&q);
        }
    }

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

    /* Overlay pass: dropdowny COMBO sa zawsze najwyzsza warstwa UI. */
    for(i=0;i<HOST_MAX_UI_CONTROLS;i++)
        if(g_host_ui_controls[i].used&&g_host_ui_controls[i].window_handle==u->handle&&
           g_host_ui_controls[i].spec.kind==NYOTA_UI_CTRL_COMBO&&g_host_ui_controls[i].combo_open)
            host_ui_draw_combo_popup(i);

    if(u->ren) SDL_RenderPresent(u->ren);
}

static void host_ui_mark_dirty(int window_index) {
    if(window_index<0||window_index>=HOST_MAX_UI_WINDOWS||!g_ui_windows[window_index].used)return;
    g_ui_windows[window_index].dirty=1;
}

static void host_ui_render_window(int window_index) {
    HostUiWindow *u;
    if(window_index<0||window_index>=HOST_MAX_UI_WINDOWS||!g_ui_windows[window_index].used)return;
    u=&g_ui_windows[window_index];
    if(!u->ren)return;
    if(u->has_background) {
        host_ui_render_background_index(window_index);
    } else {
        SDL_SetRenderDrawColor(u->ren,0,0,0,255);
        SDL_RenderClear(u->ren);
    }
    host_ui_redraw_controls(window_index);
    u->dirty=0;
}

static void host_ui_flush_dirty(void) {
    int i;
    if(g_ui_initializing)return;
    for(i=0;i<HOST_MAX_UI_WINDOWS;i++) {
        if(!g_ui_windows[i].used||!g_ui_windows[i].dirty)continue;
        host_ui_render_window(i);
        if(!g_ui_windows[i].shown&&g_ui_windows[i].win) {
            SDL_ShowWindow(g_ui_windows[i].win);
            g_ui_windows[i].shown=1;
        }
    }
}

static void host_ui_finish_initial_build(void) {
    int i;

    /* Pierwsza kompletna klatka powstaje jeszcze przy ukrytym oknie. */
    for(i=0;i<HOST_MAX_UI_WINDOWS;i++)
        if(g_ui_windows[i].used&&g_ui_windows[i].dirty)
            host_ui_render_window(i);

    /* Fedora 44 uzywa sdl2-compat/Wayland. Po mapowaniu okna wymuszamy nowa
       kompletna klatke — present wykonany tylko przed SDL_ShowWindow() nie
       gwarantuje, ze compositor od razu dostanie zawartosc. */
    for(i=0;i<HOST_MAX_UI_WINDOWS;i++) {
        if(!g_ui_windows[i].used||g_ui_windows[i].shown||!g_ui_windows[i].win)continue;
        SDL_ShowWindow(g_ui_windows[i].win);
        SDL_RaiseWindow(g_ui_windows[i].win);
        g_ui_windows[i].shown=1;
        g_ui_windows[i].dirty=1;
    }

    SDL_PumpEvents();
    g_ui_initializing=0;
    host_ui_flush_dirty();
}

static int32_t host_ui_control_create(const char *name, int32_t window_handle,
                                      int32_t parent_control_handle,
                                      const NyotaUiControlSpec *spec) {
    int i, wi = host_ui_index_by_handle(window_handle);
    (void)name;
    if (wi < 0 || !spec || !spec->w || !spec->h) return -1;
    if (spec->position_mode==NYOTA_UI_POS_AUTO && parent_control_handle<=0) return -1;
    if (parent_control_handle > 0) {
        int pi = host_ui_control_index_by_handle(parent_control_handle);
        if (pi < 0 || g_host_ui_controls[pi].window_handle != window_handle) return -1;
        if (spec->position_mode==NYOTA_UI_POS_AUTO &&
            g_host_ui_controls[pi].spec.layout!=NYOTA_UI_LAYOUT_ROW &&
            g_host_ui_controls[pi].spec.layout!=NYOTA_UI_LAYOUT_COL) return -1;
        if (spec->kind == NYOTA_UI_CTRL_TAB) {
            if (g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_TABS) return -1;
        } else if (g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_PANEL &&
                   g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_TAB &&
                   g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_STATBAR &&
                   g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_TOOLBAR &&
                   g_host_ui_controls[pi].spec.kind != NYOTA_UI_CTRL_FRAME) return -1;
    }
    for(i=0;i<HOST_MAX_UI_CONTROLS;i++) if(!g_host_ui_controls[i].used){
        memset(&g_host_ui_controls[i],0,sizeof(g_host_ui_controls[i]));
        g_host_ui_controls[i].used=1; g_host_ui_controls[i].handle=1000+i;
        g_host_ui_controls[i].window_handle=window_handle;
        g_host_ui_controls[i].parent_control_handle=parent_control_handle;
        g_host_ui_controls[i].spec=*spec;
        g_host_ui_controls[i].combo_hover=-1;
        g_host_ui_controls[i].list_hover=-1;
        g_host_ui_controls[i].tree_expanded_mask=spec->tree_expanded_mask;
        g_host_ui_controls[i].list_selected_mask=(spec->selected<64)?(1ULL<<spec->selected):0;
        g_host_ui_controls[i].caret=(uint32_t)strlen(spec->text);
        g_host_ui_controls[i].text_changed=0;
        g_host_ui_controls[i].changed=0;
        if(spec->kind==NYOTA_UI_CTRL_EQBOX){
            uint32_t k;
            for(k=0;k<NYOTA_UI_EQ_MAX_BARS;k++){
                uint32_t v=k<spec->eq_value_count?spec->eq_values[k]:spec->range_min;
                g_host_ui_controls[i].eq_peak_value[k]=v;
                g_host_ui_controls[i].eq_peak_tick[k]=host_ticks();
            }
        }
        host_ui_mark_dirty(wi);
        return g_host_ui_controls[i].handle;
    }
    return -1;
}

static int32_t host_ui_control_update(int32_t handle, const NyotaUiControlSpec *spec) {
    int i=host_ui_control_index_by_handle(handle), wi;
    if(i<0||!spec)return -1;
    host_ui_clear_eq_cache(&g_host_ui_controls[i]);
    host_ui_clear_asset_cache(&g_host_ui_controls[i]);
    g_host_ui_controls[i].spec=*spec;
    if(spec->kind==NYOTA_UI_CTRL_TREEVIEW)g_host_ui_controls[i].tree_expanded_mask=spec->tree_expanded_mask;
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);
    if(wi<0)return -1;
    host_ui_mark_dirty(wi);
    return 0;
}

static void host_ui_control_destroy_index(int idx) {
    int i, wi;
    int32_t handle;
    if(idx<0||idx>=HOST_MAX_UI_CONTROLS||!g_host_ui_controls[idx].used)return;
    handle=g_host_ui_controls[idx].handle;
    wi=host_ui_index_by_handle(g_host_ui_controls[idx].window_handle);
    host_ui_clear_eq_cache(&g_host_ui_controls[idx]);
    host_ui_clear_asset_cache(&g_host_ui_controls[idx]);
    for(i=0;i<HOST_MAX_UI_CONTROLS;i++)
        if(g_host_ui_controls[i].used&&g_host_ui_controls[i].parent_control_handle==handle)
            host_ui_control_destroy_index(i);
    memset(&g_host_ui_controls[idx],0,sizeof(g_host_ui_controls[idx]));
    if(wi>=0)host_ui_mark_dirty(wi);
}

static void host_ui_control_destroy(int32_t handle) {
    int i=host_ui_control_index_by_handle(handle); if(i>=0)host_ui_control_destroy_index(i);
}

static int32_t host_ui_button_clicked(int32_t handle) {
    int i=host_ui_control_index_by_handle(handle);
    int32_t clicked;
    if(i<0||(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_BUTTON&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_ICONBUTTON))return -1;
    if(!g_host_ui_controls[i].spec.enabled)return 0;
    host_pump();
    clicked=g_host_ui_controls[i].clicked?1:0;
    g_host_ui_controls[i].clicked=0;
    return clicked;
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
    if(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_CBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_RADIO&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SWITCH&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_ICONBUTTON)return -1;
    host_pump();return g_host_ui_controls[i].spec.checked?1:0;
}
static int32_t host_ui_control_set_checked(int32_t handle,uint8_t checked){
    int i=host_ui_control_index_by_handle(handle),j,wi;uint8_t old;if(i<0)return -1;
    if(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_CBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_RADIO&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SWITCH&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_ICONBUTTON)return -1;
    old=g_host_ui_controls[i].spec.checked;g_host_ui_controls[i].spec.checked=checked?1:0;
    if(old!=g_host_ui_controls[i].spec.checked&&(g_host_ui_controls[i].spec.kind==NYOTA_UI_CTRL_SWITCH||g_host_ui_controls[i].spec.kind==NYOTA_UI_CTRL_ICONBUTTON))g_host_ui_controls[i].changed=1;
    if(checked&&g_host_ui_controls[i].spec.kind==NYOTA_UI_CTRL_RADIO&&g_host_ui_controls[i].spec.radio_group[0])
        for(j=0;j<HOST_MAX_UI_CONTROLS;j++)if(j!=i&&g_host_ui_controls[j].used&&g_host_ui_controls[j].spec.kind==NYOTA_UI_CTRL_RADIO&&g_host_ui_controls[j].window_handle==g_host_ui_controls[i].window_handle&&!strcmp(g_host_ui_controls[j].spec.radio_group,g_host_ui_controls[i].spec.radio_group))g_host_ui_controls[j].spec.checked=0;
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);if(wi>=0)host_ui_mark_dirty(wi);return 0;
}
static int32_t host_ui_combo_index(int32_t handle){int i=host_ui_control_index_by_handle(handle);if(i<0||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_COMBO)return -1;host_pump();return (int32_t)g_host_ui_controls[i].spec.selected;}
static int32_t host_ui_combo_set_index(int32_t handle,uint32_t index){int i=host_ui_control_index_by_handle(handle),wi;uint32_t count=0,k;if(i<0||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_COMBO)return -1;for(k=0;g_host_ui_controls[i].spec.items[k];k++)if(g_host_ui_controls[i].spec.items[k]=='\n')count++;if(index>=count)return -1;g_host_ui_controls[i].spec.selected=index;wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);if(wi>=0)host_ui_mark_dirty(wi);return 0;}

static int32_t host_ui_tarea_get_text(int32_t handle,char *out,uint32_t cap,uint32_t *out_size){
    int i=host_ui_control_index_by_handle(handle);size_t n;
    if(out_size)*out_size=0;
    if(i<0||(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_TAREA&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_TBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_INPUT)||!out||!cap)return -1;
    host_pump();n=strlen(g_host_ui_controls[i].spec.text);if(n+1>cap)return -2;
    memcpy(out,g_host_ui_controls[i].spec.text,n+1);if(out_size)*out_size=(uint32_t)n;return 0;
}
static int32_t host_ui_tarea_set_text(int32_t handle,const char *text){
    int i=host_ui_control_index_by_handle(handle),wi;
    if(i<0||(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_TAREA&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_TBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_INPUT)||!text)return -1;
    if(strlen(text)>=sizeof(g_host_ui_controls[i].spec.text))return -2;
    if((g_host_ui_controls[i].spec.kind==NYOTA_UI_CTRL_TBOX||g_host_ui_controls[i].spec.kind==NYOTA_UI_CTRL_INPUT)&&(strlen(text)>g_host_ui_controls[i].spec.max_length||strchr(text,'\n')||strchr(text,'\r')))return -2;
    if(g_host_ui_controls[i].spec.kind==NYOTA_UI_CTRL_INPUT&&text[0]&&!host_ui_input_partial_valid(&g_host_ui_controls[i],text))return -2;
    strncpy(g_host_ui_controls[i].spec.text,text,sizeof(g_host_ui_controls[i].spec.text)-1);
    g_host_ui_controls[i].spec.text[sizeof(g_host_ui_controls[i].spec.text)-1]='\0';
    g_host_ui_controls[i].caret=(uint32_t)strlen(g_host_ui_controls[i].spec.text);
    g_host_ui_controls[i].text_changed=1;
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);
    if(wi>=0)host_ui_mark_dirty(wi);
    return 0;
}
static int32_t host_ui_tarea_changed(int32_t handle){
    int i=host_ui_control_index_by_handle(handle);int32_t v;
    if(i<0||(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_TAREA&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_TBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_INPUT))return -1;
    host_pump();v=g_host_ui_controls[i].text_changed?1:0;g_host_ui_controls[i].text_changed=0;return v;
}

static int32_t host_ui_range_value(int32_t handle){
    int i=host_ui_control_index_by_handle(handle);
    if(i<0||(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SBAR&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_PBAR&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SLIDER&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SPLITTER))return -1;
    host_pump();
    return (int32_t)g_host_ui_controls[i].spec.range_value;
}

static int32_t host_ui_range_set_value(int32_t handle,uint32_t value){
    int i=host_ui_control_index_by_handle(handle),wi;
    if(i<0||(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SBAR&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_PBAR&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SLIDER&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SPLITTER))return -1;
    if(value<g_host_ui_controls[i].spec.range_min||value>g_host_ui_controls[i].spec.range_max)return -2;
    g_host_ui_controls[i].spec.range_value=value;
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);
    if(wi>=0)host_ui_mark_dirty(wi);
    return 0;
}


static int32_t host_ui_signed_value(int32_t handle,int32_t *value){
    int i=host_ui_control_index_by_handle(handle);
    if(i<0||!value||(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SPINBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SCALE))return -1;
    host_pump();*value=g_host_ui_controls[i].spec.signed_value;return 0;
}
static int32_t host_ui_signed_set_value(int32_t handle,int32_t value){
    int i=host_ui_control_index_by_handle(handle),wi;HostUiControl *c;
    if(i<0)return -1;c=&g_host_ui_controls[i];
    if(c->spec.kind!=NYOTA_UI_CTRL_SPINBOX&&c->spec.kind!=NYOTA_UI_CTRL_SCALE)return -1;
    if(c->spec.kind==NYOTA_UI_CTRL_SCALE&&c->spec.scale_wrap)value=host_ui_wrap_signed(value,c->spec.signed_min,c->spec.signed_max);
    if(value<c->spec.signed_min||value>c->spec.signed_max||(c->spec.kind==NYOTA_UI_CTRL_SCALE&&c->spec.scale_wrap&&value>=c->spec.signed_max))return -2;
    if(c->spec.signed_value!=value){c->spec.signed_value=value;c->changed=1;}
    wi=host_ui_index_by_handle(c->window_handle);if(wi>=0)host_ui_mark_dirty(wi);return 0;
}
static int32_t host_ui_control_changed(int32_t handle){
    int i=host_ui_control_index_by_handle(handle);int32_t v;
    if(i<0)return -1;
    if(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SPINBOX&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SCALE&&
       g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SPLITTER&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_LISTVIEW&&
       g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_TREEVIEW&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_SWITCH&&
       g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_ICONBUTTON)return -1;
    host_pump();v=g_host_ui_controls[i].changed?1:0;g_host_ui_controls[i].changed=0;return v;
}
static int32_t host_ui_select_index(int32_t handle){
    int i=host_ui_control_index_by_handle(handle);
    if(i<0||(g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_LISTVIEW&&g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_TREEVIEW))return -1;
    host_pump();return (int32_t)g_host_ui_controls[i].spec.selected;
}
static int32_t host_ui_select_set_index(int32_t handle,uint32_t index){
    int i=host_ui_control_index_by_handle(handle),wi;HostUiControl *c;uint32_t count;
    if(i<0)return -1;c=&g_host_ui_controls[i];
    if(c->spec.kind!=NYOTA_UI_CTRL_LISTVIEW&&c->spec.kind!=NYOTA_UI_CTRL_TREEVIEW)return -1;
    count=host_ui_combo_item_count(c);if(index>=count)return -2;
    if(c->spec.selected!=index){c->spec.selected=index;c->changed=1;}
    c->list_selected_mask=index<64?(1ULL<<index):0;
    wi=host_ui_index_by_handle(c->window_handle);if(wi>=0)host_ui_mark_dirty(wi);return 0;
}
static int32_t host_ui_clock_set_values(int32_t handle,const int32_t *values,uint32_t count){
    int i=host_ui_control_index_by_handle(handle),wi;uint32_t k;HostUiControl *c;
    if(i<0||!values)return -1;c=&g_host_ui_controls[i];
    if(c->spec.kind!=NYOTA_UI_CTRL_CLOCK||count!=c->spec.clock_needles)return -1;
    for(k=0;k<count;k++)if(values[k]<c->spec.clock_needle_min[k]||values[k]>c->spec.clock_needle_max[k])return -2;
    memcpy(c->spec.clock_values,values,count*sizeof(int32_t));c->spec.clock_value_count=count;
    wi=host_ui_index_by_handle(c->window_handle);if(wi>=0)host_ui_mark_dirty(wi);return 0;
}
static int32_t host_ui_clock_set_needle(int32_t handle,uint32_t index,int32_t value){
    int i=host_ui_control_index_by_handle(handle),wi;HostUiControl *c;
    if(i<0)return -1;c=&g_host_ui_controls[i];
    if(c->spec.kind!=NYOTA_UI_CTRL_CLOCK||index>=c->spec.clock_needles)return -1;
    if(value<c->spec.clock_needle_min[index]||value>c->spec.clock_needle_max[index])return -2;
    c->spec.clock_values[index]=value;if(c->spec.clock_value_count<=index)c->spec.clock_value_count=index+1;
    wi=host_ui_index_by_handle(c->window_handle);if(wi>=0)host_ui_mark_dirty(wi);return 0;
}
static int32_t host_ui_clock_value(int32_t handle,uint32_t index,int32_t *value){
    int i=host_ui_control_index_by_handle(handle);HostUiControl *c;
    if(i<0||!value)return -1;c=&g_host_ui_controls[i];
    if(c->spec.kind!=NYOTA_UI_CTRL_CLOCK||index>=c->spec.clock_needles)return -1;
    host_pump();*value=c->spec.clock_values[index];return 0;
}

static int32_t host_ui_eqbox_set_values(int32_t handle,const uint32_t *values,uint32_t count){
    int i=host_ui_control_index_by_handle(handle),wi;uint32_t k;
    if(i<0||!values||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_EQBOX||count!=g_host_ui_controls[i].spec.eq_bars)return -1;
    for(k=0;k<count;k++)if(values[k]<g_host_ui_controls[i].spec.range_min||values[k]>g_host_ui_controls[i].spec.range_max)return -2;
    memcpy(g_host_ui_controls[i].spec.eq_values,values,count*sizeof(uint32_t));g_host_ui_controls[i].spec.eq_value_count=count;
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);if(wi>=0)host_ui_mark_dirty(wi);return 0;
}
static int32_t host_ui_eqbox_set_bar(int32_t handle,uint32_t index,uint32_t value){
    int i=host_ui_control_index_by_handle(handle),wi;
    if(i<0||g_host_ui_controls[i].spec.kind!=NYOTA_UI_CTRL_EQBOX||index>=g_host_ui_controls[i].spec.eq_bars)return -1;
    if(value<g_host_ui_controls[i].spec.range_min||value>g_host_ui_controls[i].spec.range_max)return -2;
    g_host_ui_controls[i].spec.eq_values[index]=value;
    if(g_host_ui_controls[i].spec.eq_value_count<=index)g_host_ui_controls[i].spec.eq_value_count=index+1;
    wi=host_ui_index_by_handle(g_host_ui_controls[i].window_handle);if(wi>=0)host_ui_mark_dirty(wi);return 0;
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
    flags |= SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");
    win = SDL_CreateWindow(name, sx, sy, (int)w, (int)h, flags);
    if (!win) return -1;
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren) { SDL_DestroyWindow(win); return -1; }
    SDL_RenderSetLogicalSize(ren,(int)w,(int)h);
    SDL_RenderSetIntegerScale(ren,SDL_FALSE);

    g_ui_windows[slot].used = 1;
    g_ui_windows[slot].handle = slot + 1;
    g_ui_windows[slot].parent_handle = parent_handle;
    g_ui_windows[slot].win = win;
    g_ui_windows[slot].ren = ren;
    g_ui_windows[slot].w = w;
    g_ui_windows[slot].h = h;
    g_ui_windows[slot].has_background = 0;
    g_ui_windows[slot].background_cache = NULL;
    g_ui_windows[slot].dirty = 1;
    g_ui_windows[slot].shown = 0;
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
    if(g_ui_windows[i].background_cache) {
        SDL_DestroyTexture(g_ui_windows[i].background_cache);
        g_ui_windows[i].background_cache = NULL;
    }
    g_ui_windows[i].background = *background;
    g_ui_windows[i].has_background = 1;
    host_ui_mark_dirty(i);
    return 0;
}

static void host_ui_destroy_index(int idx) {
    int i;
    int32_t handle;
    if (idx < 0 || idx >= HOST_MAX_UI_WINDOWS || !g_ui_windows[idx].used) return;
    handle = g_ui_windows[idx].handle;
    for (i = 0; i < HOST_MAX_UI_CONTROLS; i++)
        if (g_host_ui_controls[i].used && g_host_ui_controls[i].window_handle == handle) {
            host_ui_clear_eq_cache(&g_host_ui_controls[i]);
            memset(&g_host_ui_controls[i], 0, sizeof(g_host_ui_controls[i]));
        }
    for (i = 0; i < HOST_MAX_UI_WINDOWS; i++)
        if (g_ui_windows[i].used && g_ui_windows[i].parent_handle == handle)
            host_ui_destroy_index(i);
    if (g_ui_windows[idx].background_cache) {
        SDL_DestroyTexture(g_ui_windows[idx].background_cache);
        g_ui_windows[idx].background_cache = NULL;
    }
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
        for (i = 0; i < HOST_MAX_UI_FONTS; i++) {
            if (g_host_ui_fonts[i].used && g_host_ui_fonts[i].font) {
                TTF_CloseFont(g_host_ui_fonts[i].font);
                g_host_ui_fonts[i].font = NULL;
                g_host_ui_fonts[i].used = 0;
            }
        }
        if (TTF_WasInit()) TTF_Quit();
        FcFini();
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
    g_nyhost.ui_tarea_get_text = host_ui_tarea_get_text;
    g_nyhost.ui_tarea_set_text = host_ui_tarea_set_text;
    g_nyhost.ui_tarea_changed = host_ui_tarea_changed;
    g_nyhost.ui_range_value = host_ui_range_value;
    g_nyhost.ui_range_set_value = host_ui_range_set_value;
    g_nyhost.ui_eqbox_set_values = host_ui_eqbox_set_values;
    g_nyhost.ui_eqbox_set_bar = host_ui_eqbox_set_bar;
    g_nyhost.ui_signed_value = host_ui_signed_value;
    g_nyhost.ui_signed_set_value = host_ui_signed_set_value;
    g_nyhost.ui_control_changed = host_ui_control_changed;
    g_nyhost.ui_select_index = host_ui_select_index;
    g_nyhost.ui_select_set_index = host_ui_select_set_index;
    g_nyhost.ui_clock_set_values = host_ui_clock_set_values;
    g_nyhost.ui_clock_set_needle = host_ui_clock_set_needle;
    g_nyhost.ui_clock_value = host_ui_clock_value;
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
    host_ui_finish_initial_build();
    fputc('\n', stdout);
    free(src);
    wait_close();
    host_exit();
    return 0;
}
