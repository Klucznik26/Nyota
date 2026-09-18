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
    NYOTA_UI_POS_CENTER = 2
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
} NyotaHost;

void NyotaSetHost(NyotaHost *h);
void NyotaEmbedRun(const char *src, void (*emit)(char c));

#endif
