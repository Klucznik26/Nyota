/* nyota_host.h — backend hosta, nie wycinek języka.
 *
 * Nyota jest niezależnym językiem. Tunga jest osobnym edytorem.
 * AyoOS / Linux / Windows podłączają ten kontrakt.
 *
 * PRINT, GRAPH, INPUT, DELAY są poleceniami języka.
 */

#ifndef NYOTA_HOST_H
#define NYOTA_HOST_H

#include <stdint.h>
#include "nyota_color.h"

#define NYOTA_UI_MAX_GRAD_COLORS 64u
#define NYOTA_UI_PATH_MAX 512u

/* NyotaUI: stabilny kontrakt języka z hostem. */
enum {
    NYOTA_UI_POS_SYSTEM = 0,
    NYOTA_UI_POS_XY = 1,
    NYOTA_UI_POS_CENTER = 2,
    NYOTA_UI_POS_AUTO = 3
};

enum {
    NYOTA_UI_BG_COLOR = 0,
    NYOTA_UI_BG_IMAGE = 1,
    NYOTA_UI_BG_LINEAR = 2,
    NYOTA_UI_BG_SHAPE = 3,
    NYOTA_UI_BG_SPIRAL = 4
};

enum {
    NYOTA_UI_IMG_CROP = 0,
    NYOTA_UI_IMG_FIT = 1,
    NYOTA_UI_IMG_STRETCH = 2,
    NYOTA_UI_IMG_NATIVE = 3,
    NYOTA_UI_IMG_TILE = 4
};

enum {
    NYOTA_UI_DIR_VERTICAL = 0,
    NYOTA_UI_DIR_HORIZONTAL = 1,
    NYOTA_UI_DIR_DIAG_DOWN = 2,
    NYOTA_UI_DIR_DIAG_UP = 3
};

enum {
    NYOTA_UI_SHAPE_CIRCLE = 0,
    NYOTA_UI_SHAPE_ELLIPSE = 1,
    NYOTA_UI_SHAPE_SQUARE = 2,
    NYOTA_UI_SHAPE_RECT = 3,
    NYOTA_UI_SHAPE_DIAMOND = 4,
    NYOTA_UI_SHAPE_STAR = 5,
    NYOTA_UI_SHAPE_EGG = 6
};

enum {
    NYOTA_UI_SPIRAL_CW = 0,
    NYOTA_UI_SPIRAL_CCW = 1
};

typedef struct {
    uint8_t kind;
    uint8_t image_mode;
    uint8_t direction;
    uint8_t shape;
    uint8_t center_mode;
    uint8_t spiral_direction;
    int32_t center_x;
    int32_t center_y;
    int32_t angle_deg;
    uint32_t turns;
    uint32_t color_count;
    NyotaColor colors[NYOTA_UI_MAX_GRAD_COLORS];
    char image_path[NYOTA_UI_PATH_MAX];
} NyotaUiBackground;


/* NyotaUI controls. Values are language-level and platform neutral. */
enum {
    NYOTA_UI_CTRL_BUTTON = 1,
    NYOTA_UI_CTRL_LABEL = 2,
    NYOTA_UI_CTRL_PANEL = 3,
    NYOTA_UI_CTRL_DAREA = 4,
    NYOTA_UI_CTRL_CBOX = 5,
    NYOTA_UI_CTRL_COMBO = 6,
    NYOTA_UI_CTRL_SEP = 7,
    NYOTA_UI_CTRL_RADIO = 8,
    NYOTA_UI_CTRL_TABS = 9,
    NYOTA_UI_CTRL_TAB = 10,
    NYOTA_UI_CTRL_TAREA = 11,
    NYOTA_UI_CTRL_SBAR = 12,
    NYOTA_UI_CTRL_PBAR = 13,
    NYOTA_UI_CTRL_EQBOX = 14,
    NYOTA_UI_CTRL_SLIDER = 15,
    NYOTA_UI_CTRL_STATBAR = 16,
    NYOTA_UI_CTRL_TOOLBAR = 17,
    NYOTA_UI_CTRL_TBOX = 18,
    NYOTA_UI_CTRL_SPINBOX = 19,
    NYOTA_UI_CTRL_LISTVIEW = 20,
    NYOTA_UI_CTRL_TREEVIEW = 21,
    NYOTA_UI_CTRL_SPLITTER = 22,
    NYOTA_UI_CTRL_SCALE = 23,
    NYOTA_UI_CTRL_CLOCK = 24
};

enum {
    NYOTA_UI_SHADOW_OFF = 0,
    NYOTA_UI_SHADOW_R = 1,
    NYOTA_UI_SHADOW_L = 2,
    NYOTA_UI_SHADOW_U = 3,
    NYOTA_UI_SHADOW_D = 4,
    NYOTA_UI_SHADOW_RU = 5,
    NYOTA_UI_SHADOW_RD = 6,
    NYOTA_UI_SHADOW_LU = 7,
    NYOTA_UI_SHADOW_LD = 8
};

enum {
    NYOTA_UI_SEP_HORIZONTAL = 0,
    NYOTA_UI_SEP_VERTICAL = 1
};

enum {
    NYOTA_UI_SEP_NORMAL = 0,
    NYOTA_UI_SEP_INSET = 1,
    NYOTA_UI_SEP_RAISED = 2,
    NYOTA_UI_SEP_GRADIENT = 3
};

enum {
    NYOTA_UI_LAYOUT_FREE = 0,
    NYOTA_UI_LAYOUT_ROW = 1,
    NYOTA_UI_LAYOUT_COL = 2
};

enum {
    NYOTA_UI_ALIGN_LEFT = 0,
    NYOTA_UI_ALIGN_CENTER = 1,
    NYOTA_UI_ALIGN_RIGHT = 2
};

enum {
    NYOTA_UI_VALIGN_TOP = 0,
    NYOTA_UI_VALIGN_MIDDLE = 1,
    NYOTA_UI_VALIGN_BOTTOM = 2
};

enum {
    NYOTA_UI_DAREA_RECT = 0,
    NYOTA_UI_DAREA_CIRCLE = 1,
    NYOTA_UI_DAREA_ELLIPSE = 2
};

enum {
    NYOTA_UI_ACCEPT_FILES = 1,
    NYOTA_UI_ACCEPT_DIRS = 2,
    NYOTA_UI_ACCEPT_ALL = 3
};

enum {
    NYOTA_UI_SBAR_THUMB_RECT = 0,
    NYOTA_UI_SBAR_THUMB_ROUND = 1,
    NYOTA_UI_SBAR_THUMB_CIRCLE = 2,
    NYOTA_UI_SBAR_THUMB_DIAMOND = 3,
    NYOTA_UI_SBAR_THUMB_TRIANGLE = 4,
    NYOTA_UI_SBAR_THUMB_PARALLELOGRAM = 5
};

enum {
    NYOTA_UI_PBAR_SHAPE_BAR = 0,
    NYOTA_UI_PBAR_SHAPE_CIRCLE = 1,
    NYOTA_UI_PBAR_SHAPE_TRIANGLE = 2,
    NYOTA_UI_PBAR_SHAPE_SQUARE = 3,
    NYOTA_UI_PBAR_SHAPE_PARALLELOGRAM = 4
};

enum {
    NYOTA_UI_EQ_SOLID = 0,
    NYOTA_UI_EQ_CIRCLE = 1,
    NYOTA_UI_EQ_SQUARE = 2,
    NYOTA_UI_EQ_TRIANGLE = 3,
    NYOTA_UI_EQ_DIAMOND = 4,
    NYOTA_UI_EQ_PARALLELOGRAM = 5
};

enum {
    NYOTA_UI_EQ_UP = 0,
    NYOTA_UI_EQ_DOWN = 1,
    NYOTA_UI_EQ_LEFT = 2,
    NYOTA_UI_EQ_RIGHT = 3,
    NYOTA_UI_EQ_CENTER = 4
};

enum {
    NYOTA_UI_EQ_LABEL_INSIDE = 0,
    NYOTA_UI_EQ_LABEL_BOTTOM = 1,
    NYOTA_UI_EQ_LABEL_TOP = 2
};

enum {
    NYOTA_UI_SCALE_LINE = 0,
    NYOTA_UI_SCALE_ARC = 1,
    NYOTA_UI_SCALE_CIRCLE = 2
};

enum {
    NYOTA_UI_TRACK_SOLID = 0,
    NYOTA_UI_TRACK_DASH = 1,
    NYOTA_UI_TRACK_DOT = 2,
    NYOTA_UI_TRACK_SEGMENT = 3
};

enum {
    NYOTA_UI_SCALE_THUMB_RECT = 0,
    NYOTA_UI_SCALE_THUMB_ROUND = 1,
    NYOTA_UI_SCALE_THUMB_CIRCLE = 2,
    NYOTA_UI_SCALE_THUMB_DIAMOND = 3,
    NYOTA_UI_SCALE_THUMB_TRIANGLE = 4,
    NYOTA_UI_SCALE_THUMB_PARALLELOGRAM = 5,
    NYOTA_UI_SCALE_THUMB_LINE = 6,
    NYOTA_UI_SCALE_THUMB_NEEDLE = 7,
    NYOTA_UI_SCALE_THUMB_DOT = 8
};

enum {
    NYOTA_UI_THUMB_ALIGN_FIXED = 0,
    NYOTA_UI_THUMB_ALIGN_RADIAL = 1,
    NYOTA_UI_THUMB_ALIGN_TANGENT = 2
};

enum {
    NYOTA_UI_TICK_LINE = 0,
    NYOTA_UI_TICK_DOT = 1,
    NYOTA_UI_TICK_RECT = 2,
    NYOTA_UI_TICK_ROUND = 3,
    NYOTA_UI_TICK_TRIANGLE = 4,
    NYOTA_UI_TICK_DIAMOND = 5
};

enum {
    NYOTA_UI_CLOCK_ARC = 0,
    NYOTA_UI_CLOCK_CIRCLE = 1
};

enum {
    NYOTA_UI_NEEDLE_LINE = 0,
    NYOTA_UI_NEEDLE_TRIANGLE = 1,
    NYOTA_UI_NEEDLE_ARROW = 2,
    NYOTA_UI_NEEDLE_DIAMOND = 3,
    NYOTA_UI_NEEDLE_PARALLELOGRAM = 4,
    NYOTA_UI_NEEDLE_BAR = 5,
    NYOTA_UI_NEEDLE_DOUBLE = 6
};

#define NYOTA_UI_EQ_MAX_BARS 64u
#define NYOTA_UI_CLOCK_MAX_NEEDLES 8u
#define NYOTA_UI_CLOCK_MAX_ZONES 8u
#define NYOTA_UI_UNIT_MAX 32u
#define NYOTA_UI_PLACEHOLDER_MAX 256u
#define NYOTA_UI_EQ_LABEL_MAX 32u
#define NYOTA_UI_TEXT_MAX 512u
#define NYOTA_UI_FONT_MAX 128u
#define NYOTA_UI_DROP_MAX 4096u
#define NYOTA_UI_ITEMS_MAX 4096u
#define NYOTA_UI_GROUP_MAX 64u
#define NYOTA_UI_SYMBOL_MAX 32u

typedef struct {
    uint8_t kind;
    uint8_t position_mode;
    int32_t x, y;
    uint32_t w, h;

    char text[NYOTA_UI_TEXT_MAX];
    char font[NYOTA_UI_FONT_MAX];
    uint32_t font_size;
    NyotaColor text_color;
    uint8_t bold;
    uint8_t italic;
    uint8_t underline;
    uint8_t halign;
    uint8_t valign;
    uint8_t wrap;
    /* 1 = interaktywna, 0 = wyszarzona i ignoruje input. */
    uint8_t enabled;
    /* Wewnetrzny odstep tekstu kontrolki. */
    uint32_t pad_x;
    uint32_t pad_y;

    NyotaUiBackground background;
    NyotaUiBackground background_over;

    uint8_t border;
    NyotaColor border_color;
    uint32_t border_width;
    NyotaColor border_color_over;
    uint32_t border_width_over;

    uint8_t clip;
    uint8_t shape;
    uint8_t accept;
    uint8_t multi;

    /* Wspolne dla prostokatnych kontrolek. 0 = ostre rogi. */
    uint32_t radius;

    /* Layout rodzica. AUTO u dzieci korzysta z tych pol. */
    uint8_t layout;
    uint32_t gap;
    uint32_t layout_pad_x;
    uint32_t layout_pad_y;

    /* TAREA. */
    uint8_t readonly;
    NyotaColor caret_color;

    /* CBOX / RADIO. */
    uint8_t checked;
    NyotaColor check_color;
    char check_symbol[NYOTA_UI_SYMBOL_MAX];
    char radio_group[NYOTA_UI_GROUP_MAX];
    uint32_t logical_size;

    /* COMBO / TABS. Elementy COMBO sa rozdzielone '\n'. */
    char items[NYOTA_UI_ITEMS_MAX];
    uint32_t selected;
    uint32_t max_visible;
    uint8_t hover_enabled;
    NyotaUiBackground hover_background;
    NyotaColor hover_text_color;
    NyotaUiBackground drop_background;
    NyotaColor drop_text_color;
    NyotaUiBackground selected_background;
    NyotaColor selected_text_color;
    NyotaColor arrow_color;

    /* SEP / SBAR / PBAR. */
    uint8_t orientation;
    uint8_t sep_effect;
    NyotaColor sep_color;
    uint32_t sep_thickness;

    /* SBAR / PBAR — wspolny zakres wartosci. */
    uint32_t range_min;
    uint32_t range_max;
    uint32_t range_value;

    /* SBAR. */
    uint32_t range_page;
    uint32_t range_step;
    uint8_t thumb_shape;
    NyotaColor thumb_color;
    NyotaColor thumb_hover_color;

    /* PBAR. */
    uint8_t progress_shape;
    uint32_t progress_segments;
    uint32_t progress_gap;
    NyotaColor progress_fill_color;
    NyotaColor progress_empty_color;

    /* SLIDER. Uzywa wspolnego range_* oraz geometrii/kolorow thumb z SBAR. */
    uint32_t slider_thumb_size;

    /* TBOX — jednowierszowy edytor tekstu. */
    char placeholder[NYOTA_UI_PLACEHOLDER_MAX];
    uint32_t max_length;
    uint8_t password;

    /* SPINBOX — podpisana wartosc signed + przyciski +/- po prawej. */
    int32_t signed_min;
    int32_t signed_max;
    int32_t signed_value;
    int32_t signed_step;

    /* LISTVIEW/TREEVIEW. items zachowuje wspolny format wierszy rozdzielonych \n.
       TREEVIEW interpretuje element jako sciezke z segmentami rozdzielonymi '/'. */
    uint32_t row_height;
    uint32_t tree_indent;
    uint8_t show_lines;

    /* SPLITTER — interaktywny separator z wartoscia pozycji. */
    uint32_t splitter_thickness;

    /* SCALE — signed, liniowa/lukowa/kolowa skala z opcjonalnym WRAP. */
    uint8_t scale_shape;
    uint8_t scale_wrap;
    uint8_t scale_interactive;
    uint8_t track_style;
    uint32_t scale_radius;
    int32_t start_angle;
    int32_t end_angle;
    uint32_t track_width;
    uint32_t track_gap;
    NyotaUiBackground track_fill;
    NyotaUiBackground track_glow;
    uint32_t track_blur;
    uint8_t scale_thumb_shape;
    uint8_t thumb_rotate;
    uint8_t thumb_align;
    uint32_t scale_thumb_size;
    uint32_t scale_thumb_width;
    NyotaUiBackground scale_thumb_fill;
    uint8_t scale_thumb_border;
    NyotaColor scale_thumb_border_color;
    uint32_t scale_thumb_border_width;
    NyotaUiBackground scale_thumb_glow;
    uint32_t scale_thumb_blur;

    /* Wspolna podzialka SCALE/CLOCK. */
    int32_t major_step;
    int32_t minor_step;
    uint8_t tick_style;
    NyotaColor tick_color;
    NyotaColor minor_tick_color;
    uint32_t tick_len;
    uint32_t minor_tick_len;
    uint32_t tick_width;
    uint32_t tick_blur;
    uint8_t show_labels;
    int32_t label_step;
    int32_t label_offset;

    /* CLOCK — analogowy wskaznik, od jednej do wielu wskazowek. */
    uint8_t clock_shape;
    uint32_t clock_radius;
    uint32_t clock_needles;
    uint32_t clock_value_count;
    int32_t clock_values[NYOTA_UI_CLOCK_MAX_NEEDLES];
    int32_t clock_needle_min[NYOTA_UI_CLOCK_MAX_NEEDLES];
    int32_t clock_needle_max[NYOTA_UI_CLOCK_MAX_NEEDLES];
    uint8_t clock_needle_shape[NYOTA_UI_CLOCK_MAX_NEEDLES];
    NyotaColor clock_needle_color[NYOTA_UI_CLOCK_MAX_NEEDLES];
    uint32_t clock_needle_width[NYOTA_UI_CLOCK_MAX_NEEDLES];
    uint32_t clock_needle_len[NYOTA_UI_CLOCK_MAX_NEEDLES];
    uint32_t clock_needle_blur[NYOTA_UI_CLOCK_MAX_NEEDLES];
    uint32_t clock_color_count;
    uint32_t clock_shape_count;
    uint32_t clock_width_count;
    uint32_t clock_len_count;
    uint32_t clock_blur_count;
    uint32_t clock_min_count;
    uint32_t clock_max_count;
    uint32_t dial_blur;
    uint8_t center_dot;
    NyotaColor center_color;
    uint32_t center_size;
    uint32_t center_blur;
    uint8_t show_value;
    char unit[NYOTA_UI_UNIT_MAX];
    uint32_t zone_count;
    int32_t zone_min[NYOTA_UI_CLOCK_MAX_ZONES];
    int32_t zone_max[NYOTA_UI_CLOCK_MAX_ZONES];
    NyotaColor zone_color[NYOTA_UI_CLOCK_MAX_ZONES];

    /* EQBOX — szybki wieloslupek. Warstwy moga byc kolorem, gradientem lub IMG(). */
    uint32_t eq_bars;
    uint32_t eq_bar_width;
    uint32_t eq_gap;
    uint32_t eq_segments;
    uint32_t eq_segment_size;
    uint32_t eq_segment_gap;
    uint32_t eq_inactive_alpha;
    uint32_t eq_min_height;
    uint32_t eq_max_height;
    uint32_t eq_blur;
    uint32_t eq_peak_hold;
    uint8_t eq_build;
    uint8_t eq_direction;
    uint8_t eq_label_pos;
    uint8_t eq_show_labels;
    uint8_t eq_show_values;
    uint8_t eq_peak;
    NyotaColor eq_peak_color;
    uint32_t eq_value_count;
    uint32_t eq_label_count;
    uint32_t eq_color_count;
    NyotaUiBackground eq_bar_background;
    NyotaUiBackground eq_bar_fill;
    NyotaUiBackground eq_glow;
    uint32_t eq_values[NYOTA_UI_EQ_MAX_BARS];
    NyotaColor eq_bar_colors[NYOTA_UI_EQ_MAX_BARS];
    uint8_t eq_bar_color_set[NYOTA_UI_EQ_MAX_BARS];
    char eq_labels[NYOTA_UI_EQ_MAX_BARS][NYOTA_UI_EQ_LABEL_MAX];

    /* TAB: naglowek ma osobny wyglad; zawartosc uzywa pol PANEL. */
    NyotaUiBackground tab_background;
    char tab_font[NYOTA_UI_FONT_MAX];
    uint32_t tab_font_size;
    NyotaColor tab_text_color;
    uint8_t tab_bold;
    uint8_t tab_italic;
    uint8_t tab_underline;
    uint8_t tab_border;
    NyotaColor tab_border_color;
    uint32_t tab_border_width;
    uint32_t tab_radius;
    uint32_t tab_pad_x;
    uint32_t tab_pad_y;
    uint32_t tab_index;

    /* Cien dziedziczony z WIN.CONFIG przez wszystkie kontrolki okna. */
    uint8_t shadow;
    NyotaColor shadow_color;
    uint32_t shadow_depth;
} NyotaUiControlSpec;


typedef struct NyotaHost {
    void (*emit)(char c);
    void (*emit_str)(const char *s);

    uint64_t (*unix_time)(void);
    uint32_t (*local_time_seconds)(void);
    uint64_t (*ticks_100hz)(void);

    uint8_t (*wait_key)(void);
    uint8_t (*key_mods)(void);
    /* Zwraca maskę przycisków wskaźnika: bit 0 = lewy. */
    uint8_t (*pointer_state)(int32_t *x, int32_t *y);

    /* High-level filesystem contract. Rdzeń Nyoty nie dotyka VFS/POSIX bezpośrednio.
     * 0 = sukces dla operacji, exists zwraca 0/1, size < 0 oznacza błąd.
     * dir_list zwraca nazwy rozdzielone '\n' (bez . i ..). */
    int32_t (*file_read)(const char *path, char *out, uint32_t cap, uint32_t *out_size);
    int32_t (*file_write)(const char *path, const char *data, uint32_t size, uint8_t append);
    int32_t (*file_delete)(const char *path);
    int32_t (*file_copy)(const char *src, const char *dst);
    int32_t (*file_move)(const char *src, const char *dst);
    int32_t (*file_exists)(const char *path);
    int64_t (*file_size)(const char *path);

    int32_t (*dir_create)(const char *path);
    int32_t (*dir_delete)(const char *path);
    int32_t (*dir_copy)(const char *src, const char *dst);
    int32_t (*dir_move)(const char *src, const char *dst);
    int32_t (*dir_exists)(const char *path);
    int32_t (*dir_list)(const char *path, char *out, uint32_t cap, uint32_t *out_size);

    void (*gfx_clear)(uint8_t r, uint8_t g, uint8_t b);
    void (*gfx_rect)(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     uint8_t r, uint8_t g, uint8_t b);
    void (*gfx_text)(uint32_t x, uint32_t y, const char *text,
                     uint8_t r, uint8_t g, uint8_t b, uint32_t scale);
    void (*gfx_mode)(uint32_t w, uint32_t h);

    /* Off-screen SCREEN targets. ID 0 oznacza ekran fizyczny. */
    int32_t (*gfx_screen_open)(uint32_t id, uint32_t w, uint32_t h, uint32_t color_mode);
    int32_t (*gfx_screen_set)(uint32_t id);

    /* Sprite backend. Language-level SPRITE is named state in Nyota;
     * host owns only the loaded image resource and drawing. */
    int32_t (*gfx_sprite_load)(const char *path);
    void (*gfx_sprite_free)(int32_t handle);
    void (*gfx_sprite_draw)(int32_t handle, int32_t x, int32_t y,
                            uint32_t w, uint32_t h);
    void (*gfx_sprite_draw_frame)(int32_t handle, uint32_t frame,
                                  uint32_t frame_count,
                                  int32_t x, int32_t y,
                                  uint32_t w, uint32_t h);

    /* NyotaUI WIN. handle 0 oznacza ROOT po stronie hosta. */
    int32_t (*ui_win_create)(const char *name, int32_t parent_handle,
                             uint32_t w, uint32_t h,
                             int32_t x, int32_t y,
                             uint8_t position_mode, uint8_t resizable);
    int32_t (*ui_win_set_title)(int32_t handle, const char *title);
    int32_t (*ui_win_set_icon)(int32_t handle, const char *path);
    int32_t (*ui_win_set_background)(int32_t handle, const NyotaUiBackground *background);
    void (*ui_win_destroy)(int32_t handle);

    /* NyotaUI child controls. parent_control_handle=0 means direct child of WIN. */
    int32_t (*ui_control_create)(const char *name, int32_t window_handle,
                                 int32_t parent_control_handle,
                                 const NyotaUiControlSpec *spec);
    int32_t (*ui_control_update)(int32_t handle, const NyotaUiControlSpec *spec);
    void (*ui_control_destroy)(int32_t handle);
    int32_t (*ui_button_clicked)(int32_t handle);
    int32_t (*ui_darea_dropped)(int32_t handle);
    int32_t (*ui_darea_items)(int32_t handle, char *out, uint32_t cap, uint32_t *out_size);
    int32_t (*ui_control_checked)(int32_t handle);
    int32_t (*ui_control_set_checked)(int32_t handle, uint8_t checked);
    int32_t (*ui_combo_index)(int32_t handle);
    int32_t (*ui_combo_set_index)(int32_t handle, uint32_t index);
    int32_t (*ui_tarea_get_text)(int32_t handle, char *out, uint32_t cap, uint32_t *out_size);
    int32_t (*ui_tarea_set_text)(int32_t handle, const char *text);
    int32_t (*ui_tarea_changed)(int32_t handle);
    int32_t (*ui_range_value)(int32_t handle);
    int32_t (*ui_range_set_value)(int32_t handle, uint32_t value);

    /* EQBOX ma osobna szybka sciezke aktualizacji bez przebudowy CONFIG. */
    int32_t (*ui_eqbox_set_values)(int32_t handle, const uint32_t *values, uint32_t count);
    int32_t (*ui_eqbox_set_bar)(int32_t handle, uint32_t index, uint32_t value);

    /* Signed controls: SPINBOX, SCALE. */
    int32_t (*ui_signed_value)(int32_t handle, int32_t *value);
    int32_t (*ui_signed_set_value)(int32_t handle, int32_t value);
    int32_t (*ui_control_changed)(int32_t handle);

    /* LISTVIEW/TREEVIEW selection. */
    int32_t (*ui_select_index)(int32_t handle);
    int32_t (*ui_select_set_index)(int32_t handle, uint32_t index);

    /* CLOCK — szybkie aktualizacje wielu wskazowek. */
    int32_t (*ui_clock_set_values)(int32_t handle, const int32_t *values, uint32_t count);
    int32_t (*ui_clock_set_needle)(int32_t handle, uint32_t index, int32_t value);
    int32_t (*ui_clock_value)(int32_t handle, uint32_t index, int32_t *value);
} NyotaHost;

void NyotaSetHost(NyotaHost *h);
void NyotaEmbedRun(const char *src, void (*emit)(char c));

#endif
