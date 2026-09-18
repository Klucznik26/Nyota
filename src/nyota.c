// ============================================================
// NYOTA — interpreter języka Nyota
//
// Nyota jest niezależnym językiem. Tunga jest osobnym edytorem.
// PRINT, GRAPH, INPUT, DELAY należą do języka — każdy host je realizuje.
// AyoOS / Linux / Windows: ten sam rdzeń, inny backend (nyota_host.h).
//
// Standalone AyoOS: F9 / NYOTA, źródło z A:/System/_nyotarun
// ============================================================
#include <stdint.h>
#include "nyota_host.h"

#ifndef NYOTA_EMBEDDED
#include "ayo_api.h"
static AyoAPI *g_api = 0;
#endif

static NyotaHost *g_host = 0;
static void (*g_ny_emit)(char c) = 0;

void NyotaSetHost(NyotaHost *h) { g_host = h; }

static void HostRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     uint8_t r, uint8_t g, uint8_t b) {
    if (g_host && g_host->gfx_rect) g_host->gfx_rect(x, y, w, h, r, g, b);
}

static void HostText(uint32_t x, uint32_t y, const char *text,
                     uint8_t r, uint8_t g, uint8_t b, uint32_t scale) {
    if (g_host && g_host->gfx_text) g_host->gfx_text(x, y, text, r, g, b, scale);
}

static void HostClear(uint8_t r, uint8_t g, uint8_t b) {
    if (g_host && g_host->gfx_clear) g_host->gfx_clear(r, g, b);
}

static void HostGfxMode(uint32_t w, uint32_t h) {
    if (g_host && g_host->gfx_mode) g_host->gfx_mode(w, h);
}

static int32_t HostScreenOpen(uint32_t id, uint32_t w, uint32_t h, uint32_t color_mode) {
    if (g_host && g_host->gfx_screen_open) return g_host->gfx_screen_open(id, w, h, color_mode);
    return -1;
}

static int32_t HostScreenSet(uint32_t id) {
    if (g_host && g_host->gfx_screen_set) return g_host->gfx_screen_set(id);
    return -1;
}

static int32_t HostWinCreate(const char *name, int32_t parent_handle,
                             uint32_t w, uint32_t h, int32_t x, int32_t y,
                             uint8_t position_mode, uint8_t resizable) {
    if (g_host && g_host->ui_win_create)
        return g_host->ui_win_create(name, parent_handle, w, h, x, y, position_mode, resizable);
    return -1;
}

static int32_t HostWinSetTitle(int32_t handle, const char *title) {
    if (g_host && g_host->ui_win_set_title) return g_host->ui_win_set_title(handle, title);
    return -1;
}

static int32_t HostWinSetIcon(int32_t handle, const char *path) {
    if (g_host && g_host->ui_win_set_icon) return g_host->ui_win_set_icon(handle, path);
    return -1;
}

static int32_t HostWinSetBackground(int32_t handle, const NyotaUiBackground *background) {
    if (g_host && g_host->ui_win_set_background)
        return g_host->ui_win_set_background(handle, background);
    return -1;
}

static void HostWinDestroy(int32_t handle) {
    if (g_host && g_host->ui_win_destroy) g_host->ui_win_destroy(handle);
}

static int32_t HostUiControlCreate(const char *name, int32_t window_handle,
                                   int32_t parent_control_handle,
                                   const NyotaUiControlSpec *spec) {
    if (g_host && g_host->ui_control_create)
        return g_host->ui_control_create(name, window_handle, parent_control_handle, spec);
    return -1;
}

static int32_t HostUiControlUpdate(int32_t handle, const NyotaUiControlSpec *spec) {
    if (g_host && g_host->ui_control_update)
        return g_host->ui_control_update(handle, spec);
    return -1;
}

static void HostUiControlDestroy(int32_t handle) {
    if (g_host && g_host->ui_control_destroy) g_host->ui_control_destroy(handle);
}

static int32_t HostUiButtonClicked(int32_t handle) {
    if (g_host && g_host->ui_button_clicked) return g_host->ui_button_clicked(handle);
    return -1;
}

static int32_t HostUiDareaDropped(int32_t handle) {
    if (g_host && g_host->ui_darea_dropped) return g_host->ui_darea_dropped(handle);
    return -1;
}

static int32_t HostUiDareaItems(int32_t handle, char *out, uint32_t cap, uint32_t *out_size) {
    if (g_host && g_host->ui_darea_items)
        return g_host->ui_darea_items(handle, out, cap, out_size);
    return -1;
}

static int32_t HostUiControlChecked(int32_t handle) {
    if (g_host && g_host->ui_control_checked) return g_host->ui_control_checked(handle);
    return -1;
}

static int32_t HostUiControlSetChecked(int32_t handle, uint8_t checked) {
    if (g_host && g_host->ui_control_set_checked) return g_host->ui_control_set_checked(handle, checked);
    return -1;
}

static int32_t HostUiComboIndex(int32_t handle) {
    if (g_host && g_host->ui_combo_index) return g_host->ui_combo_index(handle);
    return -1;
}

static int32_t HostUiComboSetIndex(int32_t handle, uint32_t index) {
    if (g_host && g_host->ui_combo_set_index) return g_host->ui_combo_set_index(handle, index);
    return -1;
}

static int32_t HostUiTareaGetText(int32_t handle, char *out, uint32_t cap, uint32_t *out_size) {
    if (g_host && g_host->ui_tarea_get_text) return g_host->ui_tarea_get_text(handle, out, cap, out_size);
    return -1;
}

static int32_t HostUiTareaSetText(int32_t handle, const char *text) {
    if (g_host && g_host->ui_tarea_set_text) return g_host->ui_tarea_set_text(handle, text);
    return -1;
}

static int32_t HostUiTareaChanged(int32_t handle) {
    if (g_host && g_host->ui_tarea_changed) return g_host->ui_tarea_changed(handle);
    return -1;
}

static int32_t HostUiRangeValue(int32_t handle) {
    if (g_host && g_host->ui_range_value) return g_host->ui_range_value(handle);
    return -1;
}

static int32_t HostUiRangeSetValue(int32_t handle, uint32_t value) {
    if (g_host && g_host->ui_range_set_value) return g_host->ui_range_set_value(handle, value);
    return -1;
}

static int32_t HostUiEqboxSetValues(int32_t handle, const uint32_t *values, uint32_t count) {
    if (g_host && g_host->ui_eqbox_set_values) return g_host->ui_eqbox_set_values(handle, values, count);
    return -1;
}

static int32_t HostUiEqboxSetBar(int32_t handle, uint32_t index, uint32_t value) {
    if (g_host && g_host->ui_eqbox_set_bar) return g_host->ui_eqbox_set_bar(handle, index, value);
    return -1;
}

static int32_t HostUiSignedValue(int32_t handle, int32_t *value) {
    if (g_host && g_host->ui_signed_value) return g_host->ui_signed_value(handle, value);
    return -1;
}

static int32_t HostUiSignedSetValue(int32_t handle, int32_t value) {
    if (g_host && g_host->ui_signed_set_value) return g_host->ui_signed_set_value(handle, value);
    return -1;
}

static int32_t HostUiControlChanged(int32_t handle) {
    if (g_host && g_host->ui_control_changed) return g_host->ui_control_changed(handle);
    return -1;
}

static int32_t HostUiSelectIndex(int32_t handle) {
    if (g_host && g_host->ui_select_index) return g_host->ui_select_index(handle);
    return -1;
}

static int32_t HostUiSelectSetIndex(int32_t handle, uint32_t index) {
    if (g_host && g_host->ui_select_set_index) return g_host->ui_select_set_index(handle, index);
    return -1;
}

static int32_t HostUiClockSetValues(int32_t handle, const int32_t *values, uint32_t count) {
    if (g_host && g_host->ui_clock_set_values) return g_host->ui_clock_set_values(handle, values, count);
    return -1;
}

static int32_t HostUiClockSetNeedle(int32_t handle, uint32_t index, int32_t value) {
    if (g_host && g_host->ui_clock_set_needle) return g_host->ui_clock_set_needle(handle, index, value);
    return -1;
}

static int32_t HostUiClockValue(int32_t handle, uint32_t index, int32_t *value) {
    if (g_host && g_host->ui_clock_value) return g_host->ui_clock_value(handle, index, value);
    return -1;
}

static int32_t HostSpriteLoad(const char *path) {
    if (g_host && g_host->gfx_sprite_load) return g_host->gfx_sprite_load(path);
    return -1;
}

static void HostSpriteFree(int32_t handle) {
    if (g_host && g_host->gfx_sprite_free) g_host->gfx_sprite_free(handle);
}

static int HostSpriteDraw(int32_t handle, int32_t x, int32_t y,
                          uint32_t w, uint32_t h) {
    if (!g_host || !g_host->gfx_sprite_draw) return 0;
    g_host->gfx_sprite_draw(handle, x, y, w, h);
    return 1;
}

static int HostSpriteDrawFrame(int32_t handle, uint32_t frame,
                               uint32_t frame_count,
                               int32_t x, int32_t y,
                               uint32_t w, uint32_t h) {
    if (!g_host || !g_host->gfx_sprite_draw_frame) return 0;
    g_host->gfx_sprite_draw_frame(handle, frame, frame_count, x, y, w, h);
    return 1;
}

static uint64_t HostTicks(void) {
    if (g_host && g_host->ticks_100hz) return g_host->ticks_100hz();
    return 0;
}

static uint64_t HostUnixTime(void) {
    if (g_host && g_host->unix_time) return g_host->unix_time();
    return 0;
}

static uint32_t HostLocalTimeSeconds(void) {
    if (g_host && g_host->local_time_seconds) return g_host->local_time_seconds();
    return (uint32_t)(HostUnixTime() % 86400ULL);
}

static uint8_t HostWaitKey(void) {
    if (g_host && g_host->wait_key) return g_host->wait_key();
    return 0;
}

static uint8_t HostKeyMods(void) {
    if (g_host && g_host->key_mods) return g_host->key_mods();
    return 0;
}

static uint8_t HostPointerState(int32_t *x, int32_t *y) {
    if (g_host && g_host->pointer_state) return g_host->pointer_state(x, y);
    if (x) *x = 0;
    if (y) *y = 0;
    return 0;
}

static int32_t HostFileRead(const char *path, char *out, uint32_t cap, uint32_t *out_size) {
    if (g_host && g_host->file_read) return g_host->file_read(path, out, cap, out_size);
#ifndef NYOTA_EMBEDDED
    if (g_api && g_api->ReadFile && path && out && cap > 0) {
        uint32_t got = 0;
        int32_t rc = g_api->ReadFile(path, (uint8_t *)out, cap - 1, &got);
        if (rc != 0 || got >= cap) return rc ? rc : -1;
        out[got] = '\0';
        if (out_size) *out_size = got;
        return 0;
    }
#endif
    return -1;
}

static int32_t HostFileWrite(const char *path, const char *data, uint32_t size, uint8_t append) {
    if (g_host && g_host->file_write) return g_host->file_write(path, data, size, append);
    return -1;
}

static int32_t HostFileDelete(const char *path) {
    if (g_host && g_host->file_delete) return g_host->file_delete(path);
    return -1;
}

static int32_t HostFileCopy(const char *src, const char *dst) {
    if (g_host && g_host->file_copy) return g_host->file_copy(src, dst);
    return -1;
}

static int32_t HostFileMove(const char *src, const char *dst) {
    if (g_host && g_host->file_move) return g_host->file_move(src, dst);
    return -1;
}

static int32_t HostFileExists(const char *path) {
    if (g_host && g_host->file_exists) return g_host->file_exists(path);
    return 0;
}

static int64_t HostFileSize(const char *path) {
    if (g_host && g_host->file_size) return g_host->file_size(path);
    return -1;
}

static int32_t HostDirCreate(const char *path) {
    if (g_host && g_host->dir_create) return g_host->dir_create(path);
    return -1;
}

static int32_t HostDirDelete(const char *path) {
    if (g_host && g_host->dir_delete) return g_host->dir_delete(path);
    return -1;
}

static int32_t HostDirCopy(const char *src, const char *dst) {
    if (g_host && g_host->dir_copy) return g_host->dir_copy(src, dst);
    return -1;
}

static int32_t HostDirMove(const char *src, const char *dst) {
    if (g_host && g_host->dir_move) return g_host->dir_move(src, dst);
    return -1;
}

static int32_t HostDirExists(const char *path) {
    if (g_host && g_host->dir_exists) return g_host->dir_exists(path);
    return 0;
}

static int32_t HostDirList(const char *path, char *out, uint32_t cap, uint32_t *out_size) {
    if (g_host && g_host->dir_list) return g_host->dir_list(path, out, cap, out_size);
    return -1;
}

// ============================================================
// KONFIGURACJA
// ============================================================
#define SCREEN_W        1920
#define SCREEN_H        1080
#define RUN_FILE_PATH   "A:/System/_nyotarun"

#define MAX_SOURCE      32768   // max rozmiar kodu źródłowego
#define MAX_LINES       1024    // max liczba linii
#define MAX_LINE_LEN    512     // max długość linii
#define MAX_VARS        128     // max zmiennych
#define MAX_PROCS       32      // max procedur
#define MAX_CALL_DEPTH  16      // max głębokość wywołań
#define MAX_STR_LEN     512     // max długość stringa
#define MAX_LIST_ITEMS  64      // max elementów listy
#define MAX_TABLES      32      // max nazwanych kontrolek TABLE
#define MAX_TABLE_COLS  16      // max kolumn jednej TABLE
#define MAX_BUTTONS     64      // max nazwanych kontrolek BUTTON
#define MAX_SPRITES     64      // max nazwanych obiektow SPRITE
#define MAX_RECORDS     32
#define MAX_RECORD_FIELDS 16
#define MAX_SCREENS     16
#define MAX_WINDOWS     32
#define MAX_UI_CONTROLS 128
#define NYOTA_INDENT    4       // jeden poziom bloku = dokładnie 4 spacje

// ============================================================
// TYPY DANYCH
// ============================================================
#define TYPE_NONE    0
#define TYPE_INT     1
#define TYPE_FLOAT   2
#define TYPE_STR     3
#define TYPE_LIST    4
#define TYPE_BOOL    5
#define TYPE_DATE    6
#define TYPE_MARK    7
#define TYPE_TUPLE   8
#define TYPE_TIME    9
#define TYPE_RECORD  10
#define TYPE_COLOR   11

// Wartość (może być dowolnego typu)
typedef struct NyotaVal NyotaVal;
struct NyotaVal {
    uint8_t  type;
    int32_t  i;           // TYPE_INT, TYPE_BOOL, TYPE_DATE (dni od 0001.01.01)
    // float pomijamy (brak FPU w .ayo bez SSE) — approximujemy przez int*1000
    int32_t  f_int;       // część całkowita float
    int32_t  f_frac;      // część ułamkowa float (3 miejsca po przecinku *1000)
    char     s[MAX_STR_LEN];   // TYPE_STR
    struct NyotaVal *list_items;   // TYPE_LIST (wskaźnik do tablicy)
    uint32_t list_len;
    uint32_t list_cap;
};

// Zmienna
typedef struct {
    char      name[64];
    NyotaVal  val;
    uint8_t   is_const;
    uint32_t  scope;      // 0 = globalna, >0 = lokalna (głębokość wywołania)
} NyotaVar;

// Procedura
typedef struct {
    char     name[64];
    uint32_t start_line;   // linia z PROCEDURE/FUNCTION
    uint32_t body_line;    // pierwsza linia ciała
    uint32_t param_count;
    char     params[8][64];
    uint8_t  param_by_ref[8];  // czy parametr przez referencję (VAR)
    uint8_t  is_function;      // 1 = FUNCTION, 0 = PROCEDURE
    uint8_t  return_type;      // TYPE_*; blokowany po pierwszym znanym RETURN
    uint8_t  return_type_known;
} NyotaProc;

typedef struct {
    char name[64];
    uint32_t field_count;
    char fields[MAX_RECORD_FIELDS][64];
    char defaults[MAX_RECORD_FIELDS][256];
} NyotaRecordDef;

// TABLE widget support: TABLE jest kontrolką prezentacji, nie typem Nyota.
typedef struct {
    char name[64];
    int32_t x, y, w, h;
    uint32_t columns;
    int32_t widths[MAX_TABLE_COLS];
    char font[64];
    uint32_t font_size;
    uint8_t r, g, b;
    char source_name[64];
} NyotaTable;

// BUTTON jest nazwaną kontrolką GUI, a nie typem zmiennej Nyoty.
typedef struct {
    char name[64];
    int32_t x, y, w, h;
    char text[MAX_STR_LEN];
    char font[64];
    uint32_t font_size;
    uint8_t text_r, text_g, text_b;
    uint8_t bg_r, bg_g, bg_b;
    uint8_t prev_down;
} NyotaButton;

typedef struct {
    char name[64];
    char source[MAX_STR_LEN];
    int32_t x, y, w, h;
    int32_t host_handle;
    uint8_t visible;
    uint16_t frame_count;
    uint16_t frame;
    uint32_t frame_ms;
    uint8_t playing;
    uint64_t last_frame_tick;
} NyotaSprite;

typedef struct {
    char name[64];
    char parent_name[64];
    int32_t host_handle;
    uint32_t w, h;
    int32_t x, y;
    uint8_t position_mode;
    uint8_t resizable;
    char title[MAX_STR_LEN];
    char icon[MAX_STR_LEN];
    NyotaUiBackground background;
    uint8_t shadow;
    NyotaColor shadow_color;
    uint32_t shadow_depth;
} NyotaWindow;

typedef struct {
    char name[64];
    char parent_name[64];
    int32_t host_handle;
    int32_t window_handle;
    int32_t parent_control_handle;
    NyotaUiControlSpec spec;
} NyotaUiControl;

// ============================================================
// STAN GLOBALNY INTERPRETERA
// ============================================================
static uint8_t  g_source[MAX_SOURCE];
static uint32_t g_source_size = 0;

static char     g_lines[MAX_LINES][MAX_LINE_LEN];
static uint32_t g_line_count = 0;

static NyotaVar  g_vars[MAX_VARS];
static uint32_t  g_var_count = 0;

static NyotaProc g_procs[MAX_PROCS];
static uint32_t  g_proc_count = 0;

static NyotaRecordDef g_records[MAX_RECORDS];
static uint32_t g_record_count = 0;
static NyotaVar *g_with_var = 0;

static NyotaTable g_tables[MAX_TABLES];
static uint32_t   g_table_count = 0;
static NyotaButton g_buttons[MAX_BUTTONS];
static uint32_t    g_button_count = 0;
static NyotaSprite g_sprites[MAX_SPRITES];
static uint32_t    g_sprite_count = 0;
static NyotaWindow g_windows[MAX_WINDOWS];
static uint32_t    g_window_count = 0;
static NyotaUiControl g_ui_controls[MAX_UI_CONTROLS];
static uint32_t    g_ui_control_count = 0;

#define MAX_EVERY_EVENTS 16
typedef struct {
    char proc_name[64];
    uint64_t interval_ticks;
    uint64_t next_tick;
    uint8_t active;
} NyotaEveryEvent;
static NyotaEveryEvent g_every[MAX_EVERY_EVENTS];
static uint8_t g_scheduler_running = 0;

// TinyML: Mikro-Sieć Neuronowa (Perceptron bez FPU)
#define MAX_NN 4
#define MAX_NN_WEIGHTS 16
typedef struct {
    int32_t weights[MAX_NN_WEIGHTS]; // wagi skalowane x1000
    int32_t bias;                    // bias skalowany x1000
    uint32_t inputs;
    uint8_t active;
} NyotaNN;
static NyotaNN g_nn[MAX_NN];

// State Machine (Logika NPC/Gier)
#define MAX_STATES 32
static char g_states[MAX_STATES][64];
static uint8_t g_states_active[MAX_STATES];

// Stos wywołań procedur
static uint32_t g_call_stack[MAX_CALL_DEPTH];   // linia powrotu
static uint32_t g_call_depth = 0;
static uint8_t  g_in_function[MAX_CALL_DEPTH + 1];

// Bieżący wiersz wykonania
static uint32_t g_cur_line = 0;
static uint8_t  g_is_graphics = 0;  // 1 jesli wlaczono tryb GRAPH
static uint32_t g_graph_w = 0, g_graph_h = 0;
static uint8_t g_screen_exists[MAX_SCREENS];
static uint32_t g_active_screen = 0;

// Flagi kontrolne
static uint8_t  g_ny_running = 0;
static uint8_t  g_exit_flag = 0;
static uint8_t  g_continue_flag = 0;
static uint32_t g_loop_depth = 0;
static uint8_t  g_return_flag = 0;
static NyotaVal g_return_val;

// Obsługa błędów
static char     g_error[256];
static uint32_t g_error_handler_proc = 0xFFFFFFFF;  // indeks procedury ON ERROR CALL
static int32_t  g_err_code = 0;
static uint8_t  g_in_error_handler = 0;
// Lista elementów (prosta pula statyczna)
#define LIST_POOL_SIZE 2048
static NyotaVal  g_list_pool[LIST_POOL_SIZE];
static uint32_t  g_list_pool_used = 0;

// Generator liczb pseudolosowych (LCG)
static uint32_t g_rng = 0x12345678;

#define MAX_DIR_LIST_BYTES 8192
#define SORT_COUNTING_RANGE 4096
static char g_dir_list_buf[MAX_DIR_LIST_BYTES];
static uint16_t g_sort_counting[SORT_COUNTING_RANGE];
#define MAX_IMPORT_LINES 256
static char g_import_buf[MAX_SOURCE];
static char g_import_lines[MAX_IMPORT_LINES][MAX_LINE_LEN];

// Bufor wyjścia na ekran
static uint32_t  g_out_x = 0;
static uint32_t  g_out_y = 0;
#define OUT_FONT_W  16
#define OUT_FONT_H  20
#define OUT_MARGIN  8
#define OUT_MAX_X   (SCREEN_W - OUT_MARGIN * 2)
#define OUT_MAX_Y   (SCREEN_H - OUT_FONT_H - 8)

// ============================================================
// POMOCNICZE: STRING
// ============================================================
static uint32_t NStrLen(const char *s) {
    uint32_t n = 0; while (s[n]) n++; return n;
}
static void NStrCopy(char *d, const char *s, uint32_t max) {
    uint32_t i = 0;
    if (!max) return;
    while (s[i] && i + 1 < max) { d[i] = s[i]; i++; }
    d[i] = '\0';
}
static void NStrAppend(char *d, const char *s, uint32_t max) {
    uint32_t len = NStrLen(d);
    uint32_t i = 0;
    while (s[i] && len + 1 < max) d[len++] = s[i++];
    d[len] = '\0';
}
static int NStrEq(const char *a, const char *b) {
    uint32_t i = 0;
    while (a[i] && b[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static int NStrEqN(const char *a, const char *b, uint32_t n) {
    for (uint32_t i = 0; i < n; i++)
        if (a[i] != b[i] || !a[i]) return 0;
    return 1;
}
static uint8_t NIsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static uint8_t NIsDigit(char c) { return c >= '0' && c <= '9'; }
static uint8_t NIsSpace(char c) { return c == ' ' || c == '\t'; }

static void NIntToStr(int32_t v, char *buf, uint32_t max) {
    char tmp[16]; uint32_t n = 0; int neg = 0;
    if (!max) return;
    uint32_t u;
    if (v < 0) { neg = 1; u = (uint32_t)-(v + 1) + 1U; } else { u = (uint32_t)v; }
    if (u == 0) { tmp[n++] = '0'; }
    while (u && n < 15) { tmp[n++] = (char)('0' + (u % 10)); u /= 10; }
    uint32_t pos = 0;
    if (neg && pos + 1 < max) buf[pos++] = '-';
    while (n && pos + 1 < max) buf[pos++] = tmp[--n];
    buf[pos] = '\0';
}

static int32_t NStrToInt(const char *s, uint32_t *consumed) {
    int32_t v = 0; int neg = 0; uint32_t i = 0;
    while (NIsSpace(s[i])) i++;
    if (s[i] == '-') { neg = 1; i++; }
    else if (s[i] == '+') i++;
    while (NIsDigit(s[i])) { v = v * 10 + (s[i] - '0'); i++; }
    if (consumed) *consumed = i;
    return neg ? -v : v;
}

// Trim wiodących spacji — zwraca wskaźnik do pierwszego niebiałego znaku
static const char *NTrim(const char *s) {
    while (NIsSpace(*s)) s++;
    return s;
}

// Trim trailing spacji i \r (in-place)
static void NRTrim(char *s) {
    int32_t i = (int32_t)NStrLen(s) - 1;
    while (i >= 0 && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
        s[i--] = '\0';
}

// Zamień znaki na wielkie/małe litery
static void NToUpper(char *s) {
    for (uint32_t i = 0; s[i]; i++)
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = (char)(s[i] - 32);
}
static void NToLower(char *s) {
    for (uint32_t i = 0; s[i]; i++)
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = (char)(s[i] + 32);
}

// Znajdź podciąg — zwraca indeks lub -1
static int32_t NFind(const char *haystack, const char *needle) {
    uint32_t hl = NStrLen(haystack);
    uint32_t nl = NStrLen(needle);
    if (nl == 0) return 0;
    if (nl > hl) return -1;
    for (uint32_t i = 0; i <= hl - nl; i++) {
        if (NStrEqN(haystack + i, needle, nl)) return (int32_t)i;
    }
    return -1;
}

// Potęgowanie całkowite
static int32_t NPow(int32_t base, int32_t exp) {
    if (exp < 0) return 0;
    int32_t r = 1;
    while (exp-- > 0) { r *= base; }
    return r;
}

// Pierwiastek n-tego stopnia (całkowity)
static int32_t NRoot(int32_t n, int32_t root) {
    if (root <= 0) return 0;
    if (n == 0) return 0;
    if (root == 1) return n;
    int neg = 0;
    if (n < 0) {
        if (root & 1) { neg = 1; n = -n; }
        else return 0;
    }
    int32_t r = 1;
    while (NPow(r + 1, root) <= n) r++;
    return neg ? -r : r;
}

// ============================================================
// POMOCNICZE: TRYGONOMETRIA CAŁKOWITOLICZBOWA (Bhaskara I)
// ============================================================
static int32_t NSin(int32_t deg) {
    deg = deg % 360;
    if (deg < 0) deg += 360;
    if (deg == 0 || deg == 180) return 0;
    if (deg == 90) return 1000;
    if (deg == 270) return -1000;
    
    int sign = 1;
    if (deg > 180) { sign = -1; deg -= 180; }
    int32_t num = 4 * deg * (180 - deg);
    int32_t den = 40500 - deg * (180 - deg);
    return sign * ((num * 1000) / den);
}

static int32_t NCos(int32_t deg) {
    return NSin(deg + 90);
}

// ============================================================
// POMOCNICZE: SZTUCZNA INTELIGENCJA I NLP
// ============================================================
static int32_t Levenshtein(const char *s1, const char *s2) {
    uint32_t l1 = NStrLen(s1);
    uint32_t l2 = NStrLen(s2);
    if (l1 > 31) l1 = 31;
    if (l2 > 31) l2 = 31;
    uint32_t m[32][32];
    for (uint32_t i=0; i<=l1; i++) m[i][0] = i;
    for (uint32_t j=0; j<=l2; j++) m[0][j] = j;
    for (uint32_t i=1; i<=l1; i++) {
        for (uint32_t j=1; j<=l2; j++) {
            char c1 = s1[i-1]; char c2 = s2[j-1];
            if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
            if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
            uint32_t cost = (c1 == c2) ? 0 : 1;
            uint32_t a = m[i-1][j] + 1; uint32_t b = m[i][j-1] + 1;
            uint32_t c = m[i-1][j-1] + cost;
            uint32_t min = a < b ? a : b;
            m[i][j] = min < c ? min : c;
        }
    }
    uint32_t max_len = l1 > l2 ? l1 : l2;
    if (max_len == 0) return 100;
    return 100 - (m[l1][l2] * 100 / max_len);
}

static int NMatchWildcard(const char *pattern, const char *text) {
    if (*pattern == '\0') return *text == '\0';
    if (*pattern == '*') {
        pattern++;
        if (*pattern == '\0') return 1;
        while (*text != '\0') {
            if (NMatchWildcard(pattern, text)) return 1;
            text++;
        }
        return NMatchWildcard(pattern, text);
    }
    if (*pattern != *text) return 0;
    return NMatchWildcard(pattern + 1, text + 1);
}

static void NSimplify(char *s) {
    NToLower(s);
    int j = 0;
    for (int i = 0; s[i]; i++) {
        if (i > 0 && s[i] == s[i-1]) continue;
        s[j++] = s[i];
    }
    s[j] = '\0';
}

// Sprawdź czy linia zaczyna się od słowa kluczowego (oddzielonego spacją lub końcem)
static int NStartsWith(const char *line, const char *kw) {
    uint32_t klen = NStrLen(kw);
    if (!NStrEqN(line, kw, klen)) return 0;
    char next = line[klen];
    return next == '\0' || NIsSpace(next) || next == '(' || next == ':';
}

// Policz wcięcie (ilość spacji). Tabulator nie jest wcięciem.
static uint32_t NIndent(const char *line) {
    uint32_t n = 0;
    while (line[n] == ' ') n++;
    return n;
}

static int NLineHasTab(const char *line) {
    while (*line) {
        if (*line == '\t') return 1;
        line++;
    }
    return 0;
}

static NyotaVal CallNamed(const char *name, const char *paren, const char **after_out);
static uint32_t SkipBlock(uint32_t from, uint32_t block_indent);

// ============================================================
// WYJŚCIE NA EKRAN
// ============================================================
static void OutNewLine(void) {
#ifdef NYOTA_EMBEDDED
    if (g_ny_emit) { g_ny_emit('\n'); return; }
#endif
    g_out_x = OUT_MARGIN;
    g_out_y += OUT_FONT_H;
    if (g_out_y > OUT_MAX_Y) {
        // Prymitywny scroll: przesuń zawartość w górę
        // (brak memcpy w .ayo — rysujemy od nowa linię)
        g_out_y = OUT_MAX_Y;
        // Wyczyść ostatnią linię
        HostRect(0, g_out_y, SCREEN_W, OUT_FONT_H, 10, 10, 20);
    }
}

static void OutPutChar(char c) {
#ifdef NYOTA_EMBEDDED
    if (g_ny_emit) { g_ny_emit(c); return; }
#endif
    char buf[2]; buf[0] = c; buf[1] = '\0';
    if (g_out_x + OUT_FONT_W > OUT_MAX_X) OutNewLine();
    HostText(g_out_x, g_out_y, buf, 220, 220, 220, 2);
    g_out_x += OUT_FONT_W;
}

static void OutPrint(const char *s) {
    for (uint32_t i = 0; s[i]; i++) {
        // Obsługa ~/  ~//  ~/// (nyota newline markers)
        if (s[i] == '~' && s[i+1] == '/') {
            uint32_t slashes = 0;
            uint32_t j = i + 1;
            while (s[j] == '/') { slashes++; j++; }
            for (uint32_t k = 0; k < slashes; k++) OutNewLine();
            i = j - 1;
            continue;
        }
        OutPutChar(s[i]);
    }
}

static void OutPrintLine(const char *s) {
    OutPrint(s);
    OutNewLine();
}

static void OutPrintInt(int32_t v) {
    char buf[16]; NIntToStr(v, buf, sizeof(buf));
    OutPrint(buf);
}

static void OutError(const char *msg) {
    char buf[300];
    g_err_code = 1; /* 1 = błąd interpretera; host może ustawić własny kod przed OutError */
    NStrCopy(buf, "BLAD [ln.", sizeof(buf));
    {
        char num[12];
        NIntToStr((int32_t)(g_cur_line + 1), num, sizeof(num));
        NStrAppend(buf, num, sizeof(buf));
    }
    NStrAppend(buf, "]: ", sizeof(buf));
    NStrAppend(buf, msg, sizeof(buf));
#ifdef NYOTA_EMBEDDED
    if (g_ny_emit) {
        OutPrint(buf);
        OutNewLine();
    } else
#endif
    {
        HostRect(0, g_out_y, SCREEN_W, OUT_FONT_H, 10, 10, 20);
        HostText(OUT_MARGIN, g_out_y, buf, 255, 80, 80, 2);
        OutNewLine();
    }

    if (g_ny_running && !g_in_error_handler &&
        g_error_handler_proc != 0xFFFFFFFF &&
        g_error_handler_proc < g_proc_count) {
        NyotaProc *p = &g_procs[g_error_handler_proc];
        if (!p->is_function && p->param_count == 0) {
            g_in_error_handler = 1;
            CallNamed(p->name, "()", 0);
            g_in_error_handler = 0;
        }
    }
}

// ============================================================
// ZMIENNE
// ============================================================
static NyotaVar *FindVar(const char *name) {
    NyotaVar *best = 0;
    for (uint32_t i = 0; i < g_var_count; i++) {
        if (!NStrEq(g_vars[i].name, name)) continue;
        if (g_vars[i].scope > g_call_depth) continue;
        if (!best || g_vars[i].scope >= best->scope)
            best = &g_vars[i];
    }
    return best;
}

static NyotaVar *CreateVar(const char *name) {
    if (g_var_count >= MAX_VARS) return 0;
    NyotaVar *v = &g_vars[g_var_count++];
    NStrCopy(v->name, name, sizeof(v->name));
    v->val.type = TYPE_NONE;
    v->val.i = 0;
    v->val.s[0] = '\0';
    v->val.list_items = 0;
    v->val.list_len = 0;
    v->val.list_cap = 0;
    v->is_const = 0;
    v->scope = g_call_depth;   // scope = bieżąca głębokość wywołań
    return v;
}

static NyotaVar *GetOrCreateVar(const char *name) {
    NyotaVar *v = FindVar(name);
    if (!v) v = CreateVar(name);
    return v;
}

static void ValClear(NyotaVal *v);
static NyotaVal *PoolAlloc(uint32_t n);
static NyotaVal Eval(const char *expr);

static NyotaRecordDef *FindRecordDef(const char *name) {
    uint32_t i;
    for (i = 0; i < g_record_count; i++)
        if (NStrEq(g_records[i].name, name)) return &g_records[i];
    return 0;
}

static int RecordFieldIndexByDef(const NyotaRecordDef *d, const char *field) {
    uint32_t i;
    if (!d) return -1;
    for (i = 0; i < d->field_count; i++)
        if (NStrEq(d->fields[i], field)) return (int)i;
    return -1;
}

static int RecordFieldIndex(const NyotaVal *v, const char *field) {
    NyotaRecordDef *d;
    if (!v || v->type != TYPE_RECORD) return -1;
    d = FindRecordDef(v->s);
    return RecordFieldIndexByDef(d, field);
}

static NyotaVal *WithFieldPtr(const char *field) {
    int idx;
    if (!g_with_var || g_with_var->val.type != TYPE_RECORD) return 0;
    idx = RecordFieldIndex(&g_with_var->val, field);
    if (idx < 0 || (uint32_t)idx >= g_with_var->val.list_len) return 0;
    return &g_with_var->val.list_items[idx];
}

static NyotaVal RecordCreateInstance(const NyotaRecordDef *d) {
    NyotaVal v;
    uint32_t i;
    ValClear(&v);
    if (!d) return v;
    v.type = TYPE_RECORD;
    NStrCopy(v.s, d->name, sizeof(v.s));
    v.list_len = d->field_count;
    v.list_cap = d->field_count;
    v.list_items = PoolAlloc(d->field_count ? d->field_count : 1);
    if (!v.list_items) {
        OutError("RECORD: pula wartosci pelna");
        ValClear(&v);
        return v;
    }
    for (i = 0; i < d->field_count; i++) {
        v.list_items[i] = Eval(d->defaults[i]);
        if (v.list_items[i].type == TYPE_NONE) {
            OutError("RECORD: nie mozna obliczyc wartosci domyslnej pola");
            ValClear(&v);
            return v;
        }
    }
    return v;
}

// ============================================================
// PARSOWANIE WARTOŚCI LITERALNYCH
// ============================================================
static void ValClear(NyotaVal *v) {
    v->type = TYPE_NONE;
    v->i = 0;
    v->f_int = 0;
    v->f_frac = 0;
    v->s[0] = '\0';
    v->list_items = 0;
    v->list_len = 0;
    v->list_cap = 0;
}

static void ValFromInt(NyotaVal *v, int32_t i) {
    ValClear(v);
    v->type = TYPE_INT; v->i = i;
}
static void ValFromBool(NyotaVal *v, int32_t b) {
    ValClear(v);
    v->type = TYPE_BOOL; v->i = b ? 1 : 0;
}
static void ValFromStr(NyotaVal *v, const char *s) {
    ValClear(v);
    v->type = TYPE_STR;
    NStrCopy(v->s, s, MAX_STR_LEN);
}
static void ValFromColor(NyotaVal *v, const NyotaColor *c) {
    ValClear(v);
    if (!c) return;
    v->type = TYPE_COLOR;
    v->i = (int32_t)NyotaColorRGBA(c);
    v->f_int = (int32_t)c->mode;
}
static int ValToColorValue(const NyotaVal *v, NyotaColor *out) {
    uint32_t rgba;
    if (!v || !out || v->type != TYPE_COLOR) return 0;
    rgba = (uint32_t)v->i;
    out->mode = (uint8_t)v->f_int;
    out->r = (uint8_t)((rgba >> 24) & 0xFFu);
    out->g = (uint8_t)((rgba >> 16) & 0xFFu);
    out->b = (uint8_t)((rgba >> 8) & 0xFFu);
    out->a = (uint8_t)(rgba & 0xFFu);
    return 1;
}
static void ValFromFloat(NyotaVal *v, int32_t ip, int32_t frac) {
    int64_t milli = (int64_t)ip * 1000LL + (int64_t)frac;
    ValClear(v);
    /* FLOAT Nyoty ma stałą precyzję 3 miejsc i bieżący zakres oparty
     * o podpisane milli w int32. Dzięki temu wszystkie działania są
     * deterministyczne także bez FPU. */
    if (milli < INT32_MIN || milli > INT32_MAX) {
        OutError("FLOAT poza zakresem -2147483.648 .. 2147483.647");
        return;
    }
    v->type = TYPE_FLOAT;
    v->f_int = (int32_t)(milli / 1000LL);
    v->f_frac = (int32_t)(milli % 1000LL);
}

static const char *ValTypeName(uint8_t t) {
    if (t == TYPE_INT) return "INTEGER";
    if (t == TYPE_FLOAT) return "FLOAT";
    if (t == TYPE_STR) return "STRING";
    if (t == TYPE_BOOL) return "BOOLEAN";
    if (t == TYPE_LIST) return "LIST";
    if (t == TYPE_DATE) return "DATE";
    if (t == TYPE_MARK) return "MARK";
    if (t == TYPE_TUPLE) return "TUPLE";
    if (t == TYPE_TIME) return "TIME";
    if (t == TYPE_RECORD) return "RECORD";
    if (t == TYPE_COLOR) return "COLOR";
    return "NONE";
}

static int DateIsLeap(int32_t y) {
    return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
}

static int DateDim(int32_t y, int32_t m) {
    static const uint8_t dim[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m < 1 || m > 12) return 0;
    if (m == 2 && DateIsLeap(y)) return 29;
    return (int)dim[m - 1];
}

static int DateValid(int32_t y, int32_t m, int32_t d) {
    if (y < 1 || y > 9999) return 0;
    if (m < 1 || m > 12) return 0;
    if (d < 1 || d > DateDim(y, m)) return 0;
    return 1;
}

static int32_t DateToSerial(int32_t y, int32_t m, int32_t d) {
    int32_t s = 0;
    int32_t year, month;
    for (year = 1; year < y; year++)
        s += DateIsLeap(year) ? 366 : 365;
    for (month = 1; month < m; month++)
        s += DateDim(y, month);
    return s + d - 1;
}

static void SerialToDate(int32_t s, int32_t *y, int32_t *m, int32_t *d) {
    int32_t year = 1;
    int32_t month = 1;
    while (year < 9999) {
        int32_t yl = DateIsLeap(year) ? 366 : 365;
        if (s < yl) break;
        s -= yl;
        year++;
    }
    while (month < 12) {
        int32_t ml = DateDim(year, month);
        if (s < ml) break;
        s -= ml;
        month++;
    }
    *y = year;
    *m = month;
    *d = s + 1;
}

static void ValFromDateSerial(NyotaVal *v, int32_t serial) {
    int32_t maxs = DateToSerial(9999, 12, 31);
    ValClear(v);
    if (serial < 0 || serial > maxs) {
        OutError("Data poza zakresem <0001.01.01> .. <9999.12.31>");
        return;
    }
    v->type = TYPE_DATE;
    v->i = serial;
}

static void ValFromTimeSeconds(NyotaVal *v, int64_t seconds) {
    seconds %= 86400;
    if (seconds < 0) seconds += 86400;
    ValClear(v);
    v->type = TYPE_TIME;
    v->i = (int32_t)seconds;
}

static int ParseDateLit(const char *p, int32_t *y, int32_t *m, int32_t *d, uint32_t *consumed) {
    int32_t yy = 0, mm = 0, dd = 0;
    int i;
    if (p[0] != '<') return 0;
    for (i = 0; i < 4; i++) { if (!NIsDigit(p[1 + i])) return 0; yy = yy * 10 + (p[1 + i] - '0'); }
    if (p[5] != '.') return 0;
    for (i = 0; i < 2; i++) { if (!NIsDigit(p[6 + i])) return 0; mm = mm * 10 + (p[6 + i] - '0'); }
    if (p[8] != '.') return 0;
    for (i = 0; i < 2; i++) { if (!NIsDigit(p[9 + i])) return 0; dd = dd * 10 + (p[9 + i] - '0'); }
    if (p[11] != '>') return 0;
    *y = yy; *m = mm; *d = dd;
    *consumed = 12;
    return 1;
}

static int32_t FloatMilli(const NyotaVal *v) {
    return v->f_int * 1000 + v->f_frac;
}

static int ValEqual(const NyotaVal *a, const NyotaVal *b) {
    if (a->type != b->type) return 0;
    if (a->type == TYPE_INT || a->type == TYPE_BOOL || a->type == TYPE_DATE || a->type == TYPE_TIME)
        return a->i == b->i;
    if (a->type == TYPE_COLOR) return a->i == b->i && a->f_int == b->f_int;
    if (a->type == TYPE_FLOAT) return FloatMilli(a) == FloatMilli(b);
    if (a->type == TYPE_STR) return NStrEq(a->s, b->s);
    if (a->type == TYPE_RECORD) {
        if (!NStrEq(a->s, b->s) || a->list_len != b->list_len) return 0;
        for (uint32_t i = 0; i < a->list_len; i++)
            if (!ValEqual(&a->list_items[i], &b->list_items[i])) return 0;
        return 1;
    }
    if (a->type == TYPE_MARK) {
        if (a->i != b->i || a->list_len != b->list_len) return 0;
        for (uint32_t i = 0; i < a->list_len; i++) {
            if (!ValEqual(&a->list_items[i], &b->list_items[i])) return 0;
        }
        return 1;
    }
    if (a->type == TYPE_LIST || a->type == TYPE_TUPLE) {
        if (a->list_len != b->list_len) return 0;
        for (uint32_t i = 0; i < a->list_len; i++) {
            if (!ValEqual(&a->list_items[i], &b->list_items[i])) return 0;
        }
        return 1;
    }
    return 0;
}

static void ValToStr(const NyotaVal *v, char *out, uint32_t max) {
    if (v->type == TYPE_INT) {
        NIntToStr(v->i, out, max);
    } else if (v->type == TYPE_BOOL) {
        NStrCopy(out, v->i ? "TRUE" : "FALSE", max);
    } else if (v->type == TYPE_STR) {
        NStrCopy(out, v->s, max);
    } else if (v->type == TYPE_FLOAT) {
        int64_t m = (int64_t)FloatMilli(v);
        int64_t a = m < 0 ? -m : m;
        char ibuf[24], frac[4];
        out[0] = '\0';
        if (m < 0) NStrAppend(out, "-", max);
        NIntToStr((int32_t)(a / 1000LL), ibuf, sizeof(ibuf));
        NStrAppend(out, ibuf, max);
        NStrAppend(out, ".", max);
        frac[0] = (char)('0' + (a / 100) % 10);
        frac[1] = (char)('0' + (a / 10) % 10);
        frac[2] = (char)('0' + a % 10);
        frac[3] = '\0';
        NStrAppend(out, frac, max);
    } else if (v->type == TYPE_COLOR) {
        static const char hex[] = "0123456789ABCDEF";
        uint32_t rgba = (uint32_t)v->i;
        uint8_t mode = (uint8_t)v->f_int;
        uint8_t r = (uint8_t)((rgba >> 24) & 0xFFu);
        uint8_t g = (uint8_t)((rgba >> 16) & 0xFFu);
        uint8_t b = (uint8_t)((rgba >> 8) & 0xFFu);
        uint8_t a = (uint8_t)(rgba & 0xFFu);
        char buf[12];
        if (mode == NYOTA_COLOR_TRANSPARENT) {
            NStrCopy(out, "TRANSPARENT", max);
        } else if (mode == NYOTA_COLOR_BACKDROP) {
            NStrCopy(out, "BACKDROP", max);
        } else {
            buf[0] = '0'; buf[1] = 'x';
            buf[2] = hex[(r >> 4) & 15]; buf[3] = hex[r & 15];
            buf[4] = hex[(g >> 4) & 15]; buf[5] = hex[g & 15];
            buf[6] = hex[(b >> 4) & 15]; buf[7] = hex[b & 15];
            if (a != 255) {
                buf[8] = hex[(a >> 4) & 15]; buf[9] = hex[a & 15]; buf[10] = '\0';
            } else buf[8] = '\0';
            NStrCopy(out, buf, max);
        }
    } else if (v->type == TYPE_TIME) {
        int32_t hh = v->i / 3600;
        int32_t mm = (v->i / 60) % 60;
        int32_t ss = v->i % 60;
        char buf[20];
        buf[0] = 'T'; buf[1] = 'I'; buf[2] = 'M'; buf[3] = 'E'; buf[4] = '(';
        buf[5] = (char)('0' + (hh / 10) % 10);
        buf[6] = (char)('0' + hh % 10);
        buf[7] = '.';
        buf[8] = (char)('0' + (mm / 10) % 10);
        buf[9] = (char)('0' + mm % 10);
        buf[10] = '.';
        buf[11] = (char)('0' + (ss / 10) % 10);
        buf[12] = (char)('0' + ss % 10);
        buf[13] = ')';
        buf[14] = '\0';
        NStrCopy(out, buf, max);
    } else if (v->type == TYPE_DATE) {
        int32_t y = 0, m = 0, d = 0;
        char buf[16];
        SerialToDate(v->i, &y, &m, &d);
        buf[0] = '<';
        buf[1] = (char)('0' + (y / 1000) % 10);
        buf[2] = (char)('0' + (y / 100) % 10);
        buf[3] = (char)('0' + (y / 10) % 10);
        buf[4] = (char)('0' + y % 10);
        buf[5] = '.';
        buf[6] = (char)('0' + (m / 10) % 10);
        buf[7] = (char)('0' + m % 10);
        buf[8] = '.';
        buf[9] = (char)('0' + (d / 10) % 10);
        buf[10] = (char)('0' + d % 10);
        buf[11] = '>';
        buf[12] = '\0';
        NStrCopy(out, buf, max);
    } else if (v->type == TYPE_RECORD) {
        NStrCopy(out, "{", max);
        NStrAppend(out, v->s, max);
        NStrAppend(out, "}", max);
    } else if (v->type == TYPE_MARK) {
        NStrCopy(out, "{MARK ", max);
        char nbuf[16];
        NIntToStr((int32_t)v->list_len, nbuf, sizeof(nbuf));
        NStrAppend(out, nbuf, max);
        NStrAppend(out, "x", max);
        NIntToStr(v->i, nbuf, sizeof(nbuf));
        NStrAppend(out, nbuf, max);
        NStrAppend(out, "}", max);
    } else if (v->type == TYPE_LIST || v->type == TYPE_TUPLE) {
        int tuple = v->type == TYPE_TUPLE;
        NStrCopy(out, tuple ? "(" : "[", max);
        for (uint32_t i = 0; i < v->list_len; i++) {
            if (i > 0) NStrAppend(out, ", ", max);
            char tmp[128];
            ValToStr(&v->list_items[i], tmp, sizeof(tmp));
            NStrAppend(out, tmp, max);
        }
        if (tuple && v->list_len == 1) NStrAppend(out, ",", max);
        NStrAppend(out, tuple ? ")" : "]", max);
    } else {
        NStrCopy(out, "nil", max);
    }
}

static int32_t ValToInt(const NyotaVal *v) {
    if (v->type == TYPE_INT || v->type == TYPE_BOOL) return v->i;
    if (v->type == TYPE_FLOAT) return v->f_int;
    if (v->type == TYPE_STR) {
        uint32_t c = 0;
        return NStrToInt(v->s, &c);
    }
    return 0;
}

static NyotaVal *PoolAlloc(uint32_t n) {
    if (!n) n = 1;
    if (g_list_pool_used + n > LIST_POOL_SIZE) return 0;
    NyotaVal *p = &g_list_pool[g_list_pool_used];
    g_list_pool_used += n;
    return p;
}

static int FindMarkRow(const NyotaVal *mark, const NyotaVal *key) {
    uint32_t i;
    if (!mark || mark->type != TYPE_MARK) return -1;
    for (i = 0; i < mark->list_len; i++) {
        if (mark->list_items[i].type == TYPE_LIST && mark->list_items[i].list_len > 0
            && ValEqual(&mark->list_items[i].list_items[0], key))
            return (int)i;
    }
    return -1;
}

static NyotaVal MarkValuesView(const NyotaVal *row) {
    NyotaVal v;
    ValClear(&v);
    if (!row || row->type != TYPE_LIST || row->list_len == 0) return v;
    v.type = TYPE_LIST;
    v.list_items = row->list_items + 1;
    v.list_len = row->list_len - 1;
    v.list_cap = (row->list_cap > 0) ? row->list_cap - 1 : v.list_len;
    return v;
}

static NyotaVal IndexValue(NyotaVal base, NyotaVal idx) {
    NyotaVal none;
    ValClear(&none);
    if (base.type == TYPE_MARK) {
        int r = FindMarkRow(&base, &idx);
        if (r < 0) {
            OutError("Brak klucza w MARK");
            return none;
        }
        return MarkValuesView(&base.list_items[r]);
    }
    if (base.type == TYPE_LIST || base.type == TYPE_TUPLE) {
        int32_t i;
        if (idx.type != TYPE_INT) {
            OutError(base.type == TYPE_TUPLE ? "Indeks TUPLE wymaga INTEGER" : "Indeks LIST wymaga INTEGER");
            return none;
        }
        i = idx.i;
        if (i >= 0 && (uint32_t)i < base.list_len)
            return base.list_items[i];
        OutError(base.type == TYPE_TUPLE ? "Indeks TUPLE poza zakresem" : "Indeks listy poza zakresem");
        return none;
    }
    if (base.type == TYPE_STR) {
        int32_t i = ValToInt(&idx);
        uint32_t slen = NStrLen(base.s);
        if (i >= 0 && (uint32_t)i < slen) {
            char tmp[2];
            tmp[0] = base.s[i];
            tmp[1] = '\0';
            ValFromStr(&none, tmp);
            return none;
        }
        OutError("String index out of range");
        return none;
    }
    OutError("Nie mozna indeksowac tego typu");
    return none;
}

static int ListFindEq(const NyotaVal *lst, const NyotaVal *x) {
    uint32_t i;
    if (!lst || lst->type != TYPE_LIST) return -1;
    for (i = 0; i < lst->list_len; i++) {
        if (ValEqual(&lst->list_items[i], x)) return (int)i;
    }
    return -1;
}

static void ListRemoveAt(NyotaVal *lst, uint32_t i) {
    uint32_t j;
    if (!lst || i >= lst->list_len) return;
    for (j = i; j + 1 < lst->list_len; j++)
        lst->list_items[j] = lst->list_items[j + 1];
    lst->list_len--;
}

static int ListEnsureCapacity(NyotaVal *lst, uint32_t need) {
    NyotaVal *items;
    uint32_t cap, i;
    if (!lst || (lst->type != TYPE_LIST && lst->type != TYPE_TUPLE)) return 0;
    if (need <= lst->list_cap) return 1;
    if (need > MAX_LIST_ITEMS) {
        OutError("Sekwencja: przekroczono limit implementacji 64 elementow");
        return 0;
    }
    cap = lst->list_cap ? lst->list_cap : 4;
    while (cap < need && cap < MAX_LIST_ITEMS) {
        uint32_t next = cap * 2;
        cap = next > MAX_LIST_ITEMS ? MAX_LIST_ITEMS : next;
    }
    items = PoolAlloc(cap);
    if (!items) {
        OutError("Pula list pelna");
        return 0;
    }
    for (i = 0; i < lst->list_len; i++) items[i] = lst->list_items[i];
    lst->list_items = items;
    lst->list_cap = cap;
    return 1;
}

static NyotaVal ListClone(const NyotaVal *src) {
    NyotaVal r;
    uint32_t i;
    ValClear(&r);
    if (!src || src->type != TYPE_LIST) return r;
    r.type = TYPE_LIST;
    r.list_cap = src->list_len ? src->list_len : 1;
    r.list_items = PoolAlloc(r.list_cap);
    if (!r.list_items) { OutError("Pula list pelna"); ValClear(&r); return r; }
    r.list_len = src->list_len;
    for (i = 0; i < src->list_len; i++)
        r.list_items[i] = src->list_items[i];
    return r;
}

static NyotaVal SeqCloneAs(const NyotaVal *src, uint8_t dst_type) {
    NyotaVal r;
    uint32_t i;
    ValClear(&r);
    if (!src || (src->type != TYPE_LIST && src->type != TYPE_TUPLE)) return r;
    if (dst_type != TYPE_LIST && dst_type != TYPE_TUPLE) return r;
    r.type = dst_type;
    r.list_cap = src->list_len ? src->list_len : 1;
    r.list_items = PoolAlloc(r.list_cap);
    if (!r.list_items) { OutError("Pula list pelna"); ValClear(&r); return r; }
    r.list_len = src->list_len;
    for (i = 0; i < src->list_len; i++) r.list_items[i] = src->list_items[i];
    return r;
}

static NyotaVal ListConcat(const NyotaVal *a, const NyotaVal *b) {
    NyotaVal r;
    uint32_t i, n;
    ValClear(&r);
    n = a->list_len + b->list_len;
    if (n > MAX_LIST_ITEMS) {
        OutError("LIST: wynik ma wiecej niz 64 elementy");
        return r;
    }
    r.type = TYPE_LIST;
    r.list_cap = n ? n : 1;
    r.list_items = PoolAlloc(r.list_cap);
    if (!r.list_items) { OutError("Pula list pelna"); ValClear(&r); return r; }
    r.list_len = 0;
    for (i = 0; i < a->list_len; i++)
        r.list_items[r.list_len++] = a->list_items[i];
    for (i = 0; i < b->list_len; i++)
        r.list_items[r.list_len++] = b->list_items[i];
    return r;
}

static NyotaVal ListSubtract(const NyotaVal *a, const NyotaVal *b) {
    NyotaVal r = ListClone(a);
    uint32_t i;
    if (r.type != TYPE_LIST) return r;
    for (i = 0; i < b->list_len; i++) {
        int f = ListFindEq(&r, &b->list_items[i]);
        if (f >= 0) ListRemoveAt(&r, (uint32_t)f);
    }
    return r;
}

static NyotaVal ListSymDiff(const NyotaVal *a, const NyotaVal *b) {
    NyotaVal left = ListSubtract(a, b);
    NyotaVal right = ListSubtract(b, a);
    if (left.type != TYPE_LIST || right.type != TYPE_LIST) {
        ValClear(&left);
        return left;
    }
    return ListConcat(&left, &right);
}

static int ValOrd(const NyotaVal *a, const NyotaVal *b) {
    if (a->type != b->type) return 0;
    if (a->type == TYPE_INT || a->type == TYPE_DATE || a->type == TYPE_TIME) {
        if (a->i < b->i) return -1;
        if (a->i > b->i) return 1;
        return 0;
    }
    if (a->type == TYPE_FLOAT) {
        int32_t x = FloatMilli(a), y = FloatMilli(b);
        if (x < y) return -1;
        if (x > y) return 1;
        return 0;
    }
    if (a->type == TYPE_STR) {
        uint32_t i = 0;
        while (a->s[i] && a->s[i] == b->s[i]) i++;
        if ((unsigned char)a->s[i] < (unsigned char)b->s[i]) return -1;
        if ((unsigned char)a->s[i] > (unsigned char)b->s[i]) return 1;
        return 0;
    }
    return 0;
}

#define LIST_SORT_AUTO      0
#define LIST_SORT_BUBBLE    1
#define LIST_SORT_INSERT    2
#define LIST_SORT_SELECT    3
#define LIST_SORT_MERGE     4
#define LIST_SORT_QUICK     5
#define LIST_SORT_HEAP      6
#define LIST_SORT_SHELL     7
#define LIST_SORT_COUNTING  8

static void ListSwap(NyotaVal *a, NyotaVal *b) {
    NyotaVal t = *a; *a = *b; *b = t;
}

static void ListReverseItems(NyotaVal *lst) {
    uint32_t i, n;
    if (!lst) return;
    n = lst->list_len;
    for (i = 0; i < n / 2; i++)
        ListSwap(&lst->list_items[i], &lst->list_items[n - 1 - i]);
}

static void ListSortBubble(NyotaVal *lst) {
    uint32_t i, j, n = lst->list_len;
    for (i = 0; i < n; i++) {
        int changed = 0;
        for (j = 1; j < n - i; j++) {
            if (ValOrd(&lst->list_items[j - 1], &lst->list_items[j]) > 0) {
                ListSwap(&lst->list_items[j - 1], &lst->list_items[j]);
                changed = 1;
            }
        }
        if (!changed) break;
    }
}

static void ListSortInsert(NyotaVal *lst) {
    uint32_t i, j;
    for (i = 1; i < lst->list_len; i++) {
        NyotaVal key = lst->list_items[i];
        j = i;
        while (j > 0 && ValOrd(&lst->list_items[j - 1], &key) > 0) {
            lst->list_items[j] = lst->list_items[j - 1];
            j--;
        }
        lst->list_items[j] = key;
    }
}

static void ListSortSelect(NyotaVal *lst) {
    uint32_t i, j, best;
    for (i = 0; i + 1 < lst->list_len; i++) {
        best = i;
        for (j = i + 1; j < lst->list_len; j++)
            if (ValOrd(&lst->list_items[j], &lst->list_items[best]) < 0) best = j;
        if (best != i) ListSwap(&lst->list_items[i], &lst->list_items[best]);
    }
}

static void ListSortMerge(NyotaVal *lst) {
    NyotaVal tmp[MAX_LIST_ITEMS];
    uint32_t width, left, n = lst->list_len;
    for (width = 1; width < n; width *= 2) {
        for (left = 0; left < n; left += width * 2) {
            uint32_t mid = left + width;
            uint32_t right = left + width * 2;
            uint32_t i = left, j, k = left;
            if (mid > n) mid = n;
            if (right > n) right = n;
            j = mid;
            while (i < mid && j < right) {
                if (ValOrd(&lst->list_items[i], &lst->list_items[j]) <= 0)
                    tmp[k++] = lst->list_items[i++];
                else
                    tmp[k++] = lst->list_items[j++];
            }
            while (i < mid) tmp[k++] = lst->list_items[i++];
            while (j < right) tmp[k++] = lst->list_items[j++];
            for (k = left; k < right; k++) lst->list_items[k] = tmp[k];
        }
    }
}

static void ListQuickRange(NyotaVal *lst, int32_t lo, int32_t hi) {
    int32_t i = lo, j = hi;
    NyotaVal pivot;
    if (lo >= hi) return;
    pivot = lst->list_items[(lo + hi) / 2];
    while (i <= j) {
        while (ValOrd(&lst->list_items[i], &pivot) < 0) i++;
        while (ValOrd(&lst->list_items[j], &pivot) > 0) j--;
        if (i <= j) {
            ListSwap(&lst->list_items[i], &lst->list_items[j]);
            i++; j--;
        }
    }
    if (lo < j) ListQuickRange(lst, lo, j);
    if (i < hi) ListQuickRange(lst, i, hi);
}

static void ListHeapify(NyotaVal *lst, uint32_t n, uint32_t i) {
    for (;;) {
        uint32_t largest = i;
        uint32_t l = i * 2 + 1, r = l + 1;
        if (l < n && ValOrd(&lst->list_items[l], &lst->list_items[largest]) > 0) largest = l;
        if (r < n && ValOrd(&lst->list_items[r], &lst->list_items[largest]) > 0) largest = r;
        if (largest == i) break;
        ListSwap(&lst->list_items[i], &lst->list_items[largest]);
        i = largest;
    }
}

static void ListSortHeap(NyotaVal *lst) {
    uint32_t n = lst->list_len, i;
    if (n < 2) return;
    for (i = n / 2; i > 0; i--) ListHeapify(lst, n, i - 1);
    for (i = n; i > 1; i--) {
        ListSwap(&lst->list_items[0], &lst->list_items[i - 1]);
        ListHeapify(lst, i - 1, 0);
    }
}

static void ListSortShell(NyotaVal *lst) {
    uint32_t n = lst->list_len, gap;
    for (gap = n / 2; gap > 0; gap /= 2) {
        uint32_t i;
        for (i = gap; i < n; i++) {
            NyotaVal temp = lst->list_items[i];
            uint32_t j = i;
            while (j >= gap && ValOrd(&lst->list_items[j - gap], &temp) > 0) {
                lst->list_items[j] = lst->list_items[j - gap];
                j -= gap;
            }
            lst->list_items[j] = temp;
        }
    }
}

static int ListSortCounting(NyotaVal *lst) {
    uint32_t i, pos = 0, range;
    int32_t minv, maxv;
    uint8_t t;
    if (!lst->list_len) return 1;
    t = lst->list_items[0].type;
    if (t != TYPE_INT) {
        OutError("SORT COUNTING wymaga LIST INTEGER");
        return 0;
    }
    minv = maxv = lst->list_items[0].i;
    for (i = 1; i < lst->list_len; i++) {
        if (lst->list_items[i].i < minv) minv = lst->list_items[i].i;
        if (lst->list_items[i].i > maxv) maxv = lst->list_items[i].i;
    }
    if ((int64_t)maxv - (int64_t)minv + 1LL > SORT_COUNTING_RANGE) {
        OutError("SORT COUNTING: zakres wartosci przekracza 4096");
        return 0;
    }
    range = (uint32_t)((int64_t)maxv - (int64_t)minv + 1LL);
    for (i = 0; i < range; i++) g_sort_counting[i] = 0;
    for (i = 0; i < lst->list_len; i++)
        g_sort_counting[(uint32_t)(lst->list_items[i].i - minv)]++;
    for (i = 0; i < range; i++) {
        uint16_t c = g_sort_counting[i];
        while (c--) {
            ValFromInt(&lst->list_items[pos++], minv + (int32_t)i);
        }
    }
    return 1;
}

static int ListSortAlgo(NyotaVal *lst, int desc, int algo) {
    if (!lst || lst->type != TYPE_LIST || lst->list_len < 2) return 1;
    if (algo == LIST_SORT_AUTO) algo = lst->list_len < 16 ? LIST_SORT_INSERT : LIST_SORT_QUICK;
    if (algo == LIST_SORT_BUBBLE) ListSortBubble(lst);
    else if (algo == LIST_SORT_INSERT) ListSortInsert(lst);
    else if (algo == LIST_SORT_SELECT) ListSortSelect(lst);
    else if (algo == LIST_SORT_MERGE) ListSortMerge(lst);
    else if (algo == LIST_SORT_QUICK) ListQuickRange(lst, 0, (int32_t)lst->list_len - 1);
    else if (algo == LIST_SORT_HEAP) ListSortHeap(lst);
    else if (algo == LIST_SORT_SHELL) ListSortShell(lst);
    else if (algo == LIST_SORT_COUNTING) {
        if (!ListSortCounting(lst)) return 0;
    } else {
        OutError("SORT: nieznany algorytm");
        return 0;
    }
    if (desc) ListReverseItems(lst);
    return 1;
}

// ============================================================
// MARK — operacje na uporządkowanych wierszach key + values
// ============================================================
static void MarkRemoveAt(NyotaVal *mark, uint32_t row) {
    uint32_t i;
    if (!mark || mark->type != TYPE_MARK || row >= mark->list_len) return;
    for (i = row + 1; i < mark->list_len; i++)
        mark->list_items[i - 1] = mark->list_items[i];
    mark->list_len--;
}

static int MarkEnsureCapacity(NyotaVal *mark, uint32_t need) {
    NyotaVal *rows;
    uint32_t cap, i;
    if (!mark || mark->type != TYPE_MARK) return 0;
    if (need <= mark->list_cap) return 1;
    if (need > MAX_LIST_ITEMS) {
        OutError("MARK: przekroczono limit implementacji 64 wierszy");
        return 0;
    }
    cap = mark->list_cap ? mark->list_cap : 4;
    while (cap < need && cap < MAX_LIST_ITEMS) {
        uint32_t next = cap * 2;
        cap = next > MAX_LIST_ITEMS ? MAX_LIST_ITEMS : next;
    }
    rows = PoolAlloc(cap);
    if (!rows) { OutError("Pula list pelna"); return 0; }
    for (i = 0; i < mark->list_len; i++) rows[i] = mark->list_items[i];
    mark->list_items = rows;
    mark->list_cap = cap;
    return 1;
}

static int MarkAppendRowCopy(NyotaVal *mark, const NyotaVal *row) {
    NyotaVal copy;
    NyotaVal *fields;
    uint32_t i;
    if (!mark || mark->type != TYPE_MARK || !row || row->type != TYPE_LIST) return 0;
    if (row->list_len != (uint32_t)mark->i + 1U) {
        OutError("MARK: niezgodna liczba kolumn wiersza");
        return 0;
    }
    if (!MarkEnsureCapacity(mark, mark->list_len + 1)) return 0;
    fields = PoolAlloc(row->list_len ? row->list_len : 1);
    if (!fields) { OutError("Pula list pelna"); return 0; }
    ValClear(&copy);
    copy.type = TYPE_LIST;
    copy.list_items = fields;
    copy.list_len = row->list_len;
    copy.list_cap = row->list_len ? row->list_len : 1;
    for (i = 0; i < row->list_len; i++) copy.list_items[i] = row->list_items[i];
    mark->list_items[mark->list_len++] = copy;
    return 1;
}

static NyotaVal MarkClone(const NyotaVal *src) {
    NyotaVal r;
    uint32_t i;
    ValClear(&r);
    if (!src || src->type != TYPE_MARK) return r;
    r.type = TYPE_MARK;
    r.i = src->i;
    for (i = 0; i < src->list_len; i++) {
        if (!MarkAppendRowCopy(&r, &src->list_items[i])) {
            ValClear(&r);
            return r;
        }
    }
    return r;
}

static NyotaVal MarkConcat(const NyotaVal *a, const NyotaVal *b) {
    NyotaVal r;
    uint32_t i;
    ValClear(&r);
    if (!a || !b || a->type != TYPE_MARK || b->type != TYPE_MARK) return r;
    if (a->i != b->i) {
        OutError("MARK + wymaga zgodnej liczby kolumn wartosci");
        return r;
    }
    r = MarkClone(a);
    if (r.type != TYPE_MARK) return r;
    for (i = 0; i < b->list_len; i++) {
        const NyotaVal *row = &b->list_items[i];
        const NyotaVal *key = &row->list_items[0];
        if (FindMarkRow(&r, key) >= 0) {
            OutError("MARK +: konflikt klucza");
            ValClear(&r);
            return r;
        }
        if (!MarkAppendRowCopy(&r, row)) { ValClear(&r); return r; }
    }
    return r;
}

static NyotaVal MarkSubtract(const NyotaVal *a, const NyotaVal *b) {
    NyotaVal r = MarkClone(a);
    uint32_t i = 0;
    if (r.type != TYPE_MARK || !b || b->type != TYPE_MARK) return r;
    while (i < r.list_len) {
        NyotaVal *row = &r.list_items[i];
        if (row->type == TYPE_LIST && row->list_len > 0 &&
            FindMarkRow(b, &row->list_items[0]) >= 0)
            MarkRemoveAt(&r, i);
        else
            i++;
    }
    return r;
}

static NyotaVal MarkSymDiff(const NyotaVal *a, const NyotaVal *b) {
    NyotaVal r;
    uint32_t i;
    ValClear(&r);
    if (!a || !b || a->type != TYPE_MARK || b->type != TYPE_MARK) return r;
    if (a->i != b->i) {
        OutError("MARK >< wymaga zgodnej liczby kolumn wartosci");
        return r;
    }
    r.type = TYPE_MARK;
    r.i = a->i;
    for (i = 0; i < a->list_len; i++) {
        const NyotaVal *row = &a->list_items[i];
        if (FindMarkRow(b, &row->list_items[0]) < 0 && !MarkAppendRowCopy(&r, row)) {
            ValClear(&r); return r;
        }
    }
    for (i = 0; i < b->list_len; i++) {
        const NyotaVal *row = &b->list_items[i];
        if (FindMarkRow(a, &row->list_items[0]) < 0 && !MarkAppendRowCopy(&r, row)) {
            ValClear(&r); return r;
        }
    }
    return r;
}

static int MarkSortableType(uint8_t t) {
    return t == TYPE_INT || t == TYPE_FLOAT || t == TYPE_STR ||
           t == TYPE_DATE || t == TYPE_TIME;
}

static const NyotaVal *MarkSortCell(const NyotaVal *mark, uint32_t row,
                                    int by_key, int32_t col) {
    const NyotaVal *r;
    if (!mark || row >= mark->list_len) return 0;
    r = &mark->list_items[row];
    if (r->type != TYPE_LIST || r->list_len == 0) return 0;
    if (by_key) return &r->list_items[0];
    if (col < 0 || col >= mark->i || (uint32_t)(col + 1) >= r->list_len) return 0;
    return &r->list_items[col + 1];
}

static int MarkSort(NyotaVal *mark, int by_key, int32_t col, int desc) {
    uint32_t i, j;
    uint8_t t = TYPE_NONE;
    if (!mark || mark->type != TYPE_MARK) return 0;
    if (!by_key && (col < 0 || col >= mark->i)) {
        OutError("SORT MARK: kolumna poza zakresem");
        return 0;
    }
    if (mark->list_len == 0) return 1;
    {
        const NyotaVal *first = MarkSortCell(mark, 0, by_key, col);
        if (!first || !MarkSortableType(first->type)) {
            OutError("SORT MARK: pole musi byc INTEGER, FLOAT, STRING, DATE albo TIME");
            return 0;
        }
        t = first->type;
    }
    for (i = 1; i < mark->list_len; i++) {
        const NyotaVal *cell = MarkSortCell(mark, i, by_key, col);
        if (!cell || cell->type != t) {
            OutError("SORT MARK: sortowane pole ma typy mieszane");
            return 0;
        }
    }
    for (i = 1; i < mark->list_len; i++) {
        NyotaVal row = mark->list_items[i];
        j = i;
        while (j > 0) {
            const NyotaVal *left = by_key ? &mark->list_items[j - 1].list_items[0]
                                           : &mark->list_items[j - 1].list_items[col + 1];
            const NyotaVal *right = by_key ? &row.list_items[0] : &row.list_items[col + 1];
            int cmp = ValOrd(left, right);
            if (desc) cmp = -cmp;
            if (cmp <= 0) break;
            mark->list_items[j] = mark->list_items[j - 1];
            j--;
        }
        mark->list_items[j] = row;
    }
    return 1;
}

static int MarkPreflightRows(uint32_t rows, uint32_t fields) {
    uint64_t need = (uint64_t)rows * (uint64_t)(fields ? fields : 1U);
    if ((uint64_t)g_list_pool_used + need > LIST_POOL_SIZE) {
        OutError("MARK: pula list za mala dla przebudowy kolumn");
        return 0;
    }
    return 1;
}

static int MarkNumericMilli64(const NyotaVal *v, int64_t *out) {
    if (v->type == TYPE_INT) { *out = (int64_t)v->i * 1000LL; return 1; }
    if (v->type == TYPE_FLOAT) { *out = (int64_t)v->f_int * 1000LL + v->f_frac; return 1; }
    return 0;
}

static void ValFromMilli64(NyotaVal *v, int64_t milli) {
    if (milli < INT32_MIN || milli > INT32_MAX) {
        OutError("Wynik FLOAT poza zakresem -2147483.648 .. 2147483.647");
        ValClear(v);
        return;
    }
    ValFromFloat(v, 0, (int32_t)milli);
}

static int MarkStat(const NyotaVal *mark, int32_t col, int kind,
                    const NyotaVal *needle, NyotaVal *out) {
    uint32_t i, j;
    const NyotaVal *cell;
    ValClear(out);
    if (!mark || mark->type != TYPE_MARK) { OutError("Statystyka wymaga MARK"); return 0; }
    if (col < 0 || col >= mark->i) { OutError("Statystyka MARK: kolumna poza zakresem"); return 0; }

    if (kind == 7) { /* COUNT */
        int32_t count = 0;
        for (i = 0; i < mark->list_len; i++) {
            cell = &mark->list_items[i].list_items[col + 1];
            if (needle && ValEqual(cell, needle)) count++;
        }
        ValFromInt(out, count);
        return 1;
    }

    if (mark->list_len == 0) { OutError("Statystyka MARK: pusty MARK"); return 0; }

    if (kind == 5 || kind == 6) { /* MODE / MODECOUNT */
        uint32_t best_i = 0, best_count = 0;
        for (i = 0; i < mark->list_len; i++) {
            uint32_t count = 0;
            const NyotaVal *cur = &mark->list_items[i].list_items[col + 1];
            for (j = 0; j < mark->list_len; j++) {
                const NyotaVal *other = &mark->list_items[j].list_items[col + 1];
                if (ValEqual(cur, other)) count++;
            }
            if (count > best_count) { best_count = count; best_i = i; }
        }
        if (kind == 5) *out = mark->list_items[best_i].list_items[col + 1];
        else ValFromInt(out, (int32_t)best_count);
        return 1;
    }

    if (kind == 3 || kind == 4) { /* MIN / MAX: liczby, DATE lub TIME */
        const NyotaVal *best = &mark->list_items[0].list_items[col + 1];
        int numeric = best->type == TYPE_INT || best->type == TYPE_FLOAT;
        int temporal = best->type == TYPE_DATE || best->type == TYPE_TIME;
        if (!numeric && !temporal) {
            OutError("MIN/MAX MARK wymaga kolumny liczbowej, DATE albo TIME");
            return 0;
        }
        for (i = 1; i < mark->list_len; i++) {
            const NyotaVal *cur = &mark->list_items[i].list_items[col + 1];
            int cmp;
            if (numeric) {
                int64_t a, b;
                if (!MarkNumericMilli64(best, &a) || !MarkNumericMilli64(cur, &b)) {
                    OutError("MIN/MAX MARK: niezgodne typy w kolumnie"); return 0;
                }
                cmp = a < b ? -1 : (a > b ? 1 : 0);
            } else {
                if (cur->type != best->type) { OutError("MIN/MAX MARK: niezgodne typy w kolumnie"); return 0; }
                cmp = ValOrd(best, cur);
            }
            if ((kind == 3 && cmp > 0) || (kind == 4 && cmp < 0)) best = cur;
        }
        *out = *best;
        return 1;
    }

    { /* SUM / AVG / MED */
        int64_t vals[MAX_LIST_ITEMS];
        int64_t sum = 0;
        int all_int = 1;
        for (i = 0; i < mark->list_len; i++) {
            const NyotaVal *cur = &mark->list_items[i].list_items[col + 1];
            if (!MarkNumericMilli64(cur, &vals[i])) {
                OutError("SUM/AVG/MED MARK wymaga kolumny INTEGER/FLOAT");
                return 0;
            }
            if (cur->type != TYPE_INT) all_int = 0;
            sum += vals[i];
        }
        if (kind == 0) { /* SUM */
            if (all_int && sum % 1000LL == 0 && sum / 1000LL >= INT32_MIN && sum / 1000LL <= INT32_MAX)
                ValFromInt(out, (int32_t)(sum / 1000LL));
            else
                ValFromMilli64(out, sum);
            return out->type != TYPE_NONE;
        }
        if (kind == 1) { /* AVG */
            ValFromMilli64(out, sum / (int64_t)mark->list_len);
            return out->type != TYPE_NONE;
        }
        /* MED */
        for (i = 1; i < mark->list_len; i++) {
            int64_t v = vals[i];
            j = i;
            while (j > 0 && vals[j - 1] > v) { vals[j] = vals[j - 1]; j--; }
            vals[j] = v;
        }
        if ((mark->list_len & 1U) && all_int) {
            ValFromInt(out, (int32_t)(vals[mark->list_len / 2] / 1000LL));
        } else if (mark->list_len & 1U) {
            ValFromMilli64(out, vals[mark->list_len / 2]);
        } else {
            ValFromMilli64(out, (vals[mark->list_len / 2 - 1] + vals[mark->list_len / 2]) / 2LL);
        }
        return out->type != TYPE_NONE;
    }
}

static int32_t TruncMilli(int32_t milli, int n) {
    int32_t div = 1;
    int i;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    for (i = 0; i < 3 - n; i++) div *= 10;
    return milli / div;
}

static int ValEqN(const NyotaVal *a, const NyotaVal *b, int n) {
    int32_t am, bm;
    if (a->type != b->type) return -1;
    if (a->type == TYPE_INT) {
        am = a->i * 1000;
        bm = b->i * 1000;
    } else if (a->type == TYPE_FLOAT) {
        am = FloatMilli(a);
        bm = FloatMilli(b);
    } else {
        return -1;
    }
    return TruncMilli(am, n) == TruncMilli(bm, n);
}

// ============================================================
// EWALUACJA WYRAŻEŃ
// ============================================================

// Forward declarations
static NyotaVal Eval(const char *expr);
static int32_t  EvalBool(const char *expr);
static NyotaVal CallNamed(const char *name, const char *paren, const char **after_out);
static NyotaVal ParseOr(const char **pp);
static NyotaVal ParsePrimary(const char **pp);
static NyotaButton *FindButton(const char *name);
static int ButtonPollClicked(NyotaButton *b);
static NyotaUiControl *FindUiControl(const char *name);
static uint32_t UiItemsCount(const char *s);
static NyotaSprite *FindSprite(const char *name);
static int SpriteArgName(const char *arg, char *out, uint32_t out_size);
static int SpriteHit(const NyotaSprite *a, const NyotaSprite *b);
static void SpriteUpdateAnimation(NyotaSprite *s);

static NyotaVal ValArith(const NyotaVal *a, char op, const NyotaVal *b) {
    NyotaVal r;
    ValClear(&r);
    if (a->type == TYPE_DATE || b->type == TYPE_DATE) {
        if (op == '+') {
            if (a->type == TYPE_DATE && b->type == TYPE_INT) {
                ValFromDateSerial(&r, a->i + b->i);
                return r;
            }
            if (a->type == TYPE_INT && b->type == TYPE_DATE) {
                ValFromDateSerial(&r, b->i + a->i);
                return r;
            }
            OutError("DATE + DATE jest niedozwolone");
            return r;
        }
        if (op == '-') {
            if (a->type == TYPE_DATE && b->type == TYPE_DATE) {
                ValFromInt(&r, a->i - b->i);
                return r;
            }
            if (a->type == TYPE_DATE && b->type == TYPE_INT) {
                ValFromDateSerial(&r, a->i - b->i);
                return r;
            }
            OutError("Niedozwolone odejmowanie od DATE");
            return r;
        }
        OutError("DATE obsluguje tylko + i -");
        return r;
    }
    if (a->type == TYPE_TIME || b->type == TYPE_TIME) {
        if (op == '-' && a->type == TYPE_TIME && b->type == TYPE_TIME) {
            ValFromInt(&r, a->i - b->i);
            return r;
        }
        OutError("TIME obsluguje przesuniecia H/M/S oraz TIME - TIME");
        return r;
    }
    if (a->type != b->type) {
        char err[160];
        NStrCopy(err, "Niezgodnosc typow: ", sizeof(err));
        NStrAppend(err, ValTypeName(a->type), sizeof(err));
        NStrAppend(err, " i ", sizeof(err));
        NStrAppend(err, ValTypeName(b->type), sizeof(err));
        OutError(err);
        return r;
    }
    if (a->type == TYPE_BOOL) {
        OutError("BOOLEAN nie uczestniczy w arytmetyce (uzyj INT)");
        return r;
    }
    if (a->type == TYPE_LIST) {
        if (op == '+') return ListConcat(a, b);
        if (op == '-') return ListSubtract(a, b);
        OutError("LIST obsluguje tylko + i -");
        return r;
    }
    if (a->type == TYPE_MARK) {
        if (op == '+') return MarkConcat(a, b);
        if (op == '-') return MarkSubtract(a, b);
        OutError("MARK obsluguje tylko + i - w arytmetyce");
        return r;
    }
    if (a->type == TYPE_STR) {
        if (op != '+') {
            OutError("STRING obsluguje tylko +");
            return r;
        }
        ValFromStr(&r, a->s);
        NStrAppend(r.s, b->s, MAX_STR_LEN);
        return r;
    }
    if (a->type == TYPE_INT) {
        int32_t lv = a->i, rv = b->i;
        int64_t wide = 0;
        if ((op == '/' || op == 'M') && rv == 0) {
            OutError(op == 'M' ? "MOD 0" : "Dzielenie przez zero");
            return r;
        }
        if ((op == '/' || op == 'M') && lv == INT32_MIN && rv == -1) {
            OutError("Przepelnienie INTEGER");
            return r;
        }
        if (op == '+') wide = (int64_t)lv + rv;
        else if (op == '-') wide = (int64_t)lv - rv;
        else if (op == '*') wide = (int64_t)lv * rv;
        else if (op == '/') { ValFromInt(&r, lv / rv); return r; }
        else if (op == 'M') { ValFromInt(&r, lv % rv); return r; }
        else if (op == '%') wide = ((int64_t)lv * rv) / 100LL;
        else if (op == '^') {
            int32_t e;
            int64_t acc = 1, base = lv;
            if (rv < 0) { OutError("Ujemny wykladnik INTEGER nie jest obslugiwany"); return r; }
            e = rv;
            while (e > 0) {
                if (e & 1) {
                    acc *= base;
                    if (acc < INT32_MIN || acc > INT32_MAX) { OutError("Przepelnienie INTEGER"); return r; }
                }
                e >>= 1;
                if (e) {
                    base *= base;
                    if (base < INT32_MIN || base > INT32_MAX) { OutError("Przepelnienie INTEGER"); return r; }
                }
            }
            ValFromInt(&r, (int32_t)acc);
            return r;
        } else if (op == 'R') {
            if (rv <= 0) { OutError("Stopien pierwiastka musi byc dodatni"); return r; }
            if (lv < 0 && (rv % 2) == 0) { OutError("Parzysty pierwiastek z liczby ujemnej"); return r; }
            ValFromInt(&r, NRoot(lv, rv));
            return r;
        } else {
            OutError("Nieznany operator");
            return r;
        }
        if (wide < INT32_MIN || wide > INT32_MAX) {
            OutError("Przepelnienie INTEGER");
            return r;
        }
        ValFromInt(&r, (int32_t)wide);
        return r;
    }
    if (a->type == TYPE_FLOAT) {
        int64_t lm = FloatMilli(a), rm = FloatMilli(b);
        int64_t result_milli = 0;
        if ((op == '/' || op == 'M') && rm == 0) {
            OutError(op == 'M' ? "MOD 0" : "Dzielenie przez zero");
            return r;
        }
        if (op == '+') result_milli = lm + rm;
        else if (op == '-') result_milli = lm - rm;
        else if (op == '*') result_milli = (lm * rm) / 1000LL;
        else if (op == '/') result_milli = (lm * 1000LL) / rm;
        else if (op == '%') result_milli = (lm * rm) / 100000LL;
        else {
            OutError("Ten operator nie obsluguje FLOAT");
            return r;
        }
        ValFromMilli64(&r, result_milli);
        return r;
    }
    OutError("Nieobslugiwany typ w dzialaniu");
    return r;
}

static int ParseNumber(const char *expr, NyotaVal *out, uint32_t *consumed) {
    uint32_t i = 0;
    int neg = 0;
    int64_t ip = 0;
    if (expr[0] == '-') { neg = 1; i++; }
    if (!NIsDigit(expr[i])) return 0;
    while (NIsDigit(expr[i])) {
        ip = ip * 10LL + (expr[i] - '0');
        if (ip > 2147483648LL) return 0;
        i++;
    }
    if (expr[i] == '.') {
        int32_t frac = 0;
        int digits = 0;
        if (!NIsDigit(expr[i + 1])) return 0;
        i++;
        while (NIsDigit(expr[i])) {
            /* FLOAT przechowuje 3 miejsca. Nadmiar cyfr jest świadomie
             * ucinany, co zachowuje przykłady =N= z 2.5678. */
            if (digits < 3) frac = frac * 10 + (expr[i] - '0');
            digits++;
            i++;
        }
        if (expr[i] == '.') return 0; /* np. 14.20.20 poza TIME() */
        while (digits < 3) { frac *= 10; digits++; }
        if (neg) { ip = -ip; frac = -frac; }
        if (ip < INT32_MIN || ip > INT32_MAX) return 0;
        ValFromFloat(out, (int32_t)ip, frac);
        if (out->type != TYPE_FLOAT) return 0;
        *consumed = i;
        return 1;
    }
    if (neg) ip = -ip;
    if (ip < INT32_MIN || ip > INT32_MAX) return 0;
    ValFromInt(out, (int32_t)ip);
    *consumed = i;
    return 1;
}

static const char *MatchParen(const char *open) {
    const char *q = open;
    if (*q != '(') return open;
    int depth = 0, in_str = 0;
    q++;
    while (*q) {
        if (*q == '"') in_str = !in_str;
        else if (!in_str && (*q == '(' || *q == '[' || *q == '{')) depth++;
        else if (!in_str && (*q == ')' || *q == ']' || *q == '}')) {
            if (*q == ')' && depth == 0) return q + 1;
            if (depth > 0) depth--;
        }
        q++;
    }
    return q;
}

static int ValTruthy(const NyotaVal *v) {
    if (v->type == TYPE_BOOL || v->type == TYPE_INT) return v->i != 0;
    if (v->type == TYPE_FLOAT) return FloatMilli(v) != 0;
    if (v->type == TYPE_STR) return NStrLen(v->s) > 0;
    if (v->type == TYPE_LIST || v->type == TYPE_TUPLE) return v->list_len > 0;
    if (v->type == TYPE_DATE || v->type == TYPE_TIME) return 1;
    return 0;
}

static NyotaVal ValCompareOp(const NyotaVal *lv, const char *op, const NyotaVal *rv) {
    NyotaVal r;
    ValClear(&r);
    if (NStrEq(op, "=") || NStrEq(op, "<>")) {
        if (lv->type != rv->type) {
            char err[160];
            NStrCopy(err, "Porownanie wymaga tego samego typu: ", sizeof(err));
            NStrAppend(err, ValTypeName(lv->type), sizeof(err));
            NStrAppend(err, " i ", sizeof(err));
            NStrAppend(err, ValTypeName(rv->type), sizeof(err));
            OutError(err);
            return r;
        }
        int eq = ValEqual(lv, rv);
        ValFromBool(&r, NStrEq(op, "=") ? eq : !eq);
        return r;
    }
    if (lv->type != rv->type ||
        (lv->type != TYPE_INT && lv->type != TYPE_FLOAT && lv->type != TYPE_DATE && lv->type != TYPE_TIME)) {
        OutError("Porownanie < > wymaga INTEGER, FLOAT, DATE albo TIME tego samego typu");
        return r;
    }
    {
        int32_t lc = (lv->type == TYPE_FLOAT) ? FloatMilli(lv) : lv->i;
        int32_t rc = (rv->type == TYPE_FLOAT) ? FloatMilli(rv) : rv->i;
        int cmp = 0;
        if (NStrEq(op, "<")) cmp = lc < rc;
        else if (NStrEq(op, ">")) cmp = lc > rc;
        else if (NStrEq(op, "<=") || NStrEq(op, "=<")) cmp = lc <= rc;
        else if (NStrEq(op, ">=") || NStrEq(op, "=>")) cmp = lc >= rc;
        ValFromBool(&r, cmp);
    }
    return r;
}

static int PeekCmpOp(const char *p, char *op, uint32_t *oplen) {
    static const char *const ops[] = {
        "<>", "><", "=<", "=>", "<=", ">=", "=", "<", ">", 0
    };
    int i;
    for (i = 0; ops[i]; i++) {
        uint32_t n = NStrLen(ops[i]);
        if (NStrEqN(p, ops[i], n)) {
            if (ops[i][0] == '=' && p[1] == '=') return 0;
            if (ops[i][0] == '=' && n == 1 && NIsDigit(p[1])) {
                uint32_t k = 1;
                while (NIsDigit(p[k])) k++;
                if (p[k] == '=') return 0;
            }
            if (ops[i][0] == '<') {
                int32_t y = 0, m = 0, d = 0;
                uint32_t cons = 0;
                if (ParseDateLit(p, &y, &m, &d, &cons)) return 0;
            }
            NStrCopy(op, ops[i], 4);
            *oplen = n;
            return 1;
        }
    }
    return 0;
}

// Parsuj string literal "..." — zwraca zawartość bez cudzysłowów
static int ParseStringLit(const char *p, char *out, uint32_t max, uint32_t *consumed) {
    if (*p != '"') return 0;
    p++; uint32_t i = 0; uint32_t n = 1;
    while (*p && *p != '"' && i + 1 < max) {
        out[i++] = *p++; n++;
    }
    out[i] = '\0';
    if (*p == '"') n++;
    *consumed = n;
    return 1;
}

// Pobierz nazwę identyfikatora
static uint32_t ParseIdent(const char *p, char *out, uint32_t max) {
    uint32_t i = 0;
    while ((NIsAlpha(p[i]) || (i > 0 && NIsDigit(p[i]))) && i + 1 < max) {
        out[i] = p[i]; i++;
    }
    out[i] = '\0';
    return i;
}

// Rozdzielacz argumentow do funkcji wbudowanych obslugujacy bloki [] i ()
static int SplitFunctionArgs(const char *p, char args[][MAX_STR_LEN], int max_args) {
    int count = 0;
    uint32_t ai = 0; int depth = 0; int in_str = 0;
    while (*p && count < max_args) {
        if (*p == '"') in_str = !in_str;
        if (!in_str && (*p == '(' || *p == '[' || *p == '{')) depth++;
        if (!in_str && (*p == ')' || *p == ']' || *p == '}')) { 
            if (depth == 0) {
                args[count][ai] = '\0'; NRTrim(args[count]); count++;
                break;
            }
            depth--; 
        }
        if (!in_str && depth == 0 && *p == ',') {
            args[count][ai] = '\0'; NRTrim(args[count]); count++;
            ai = 0; p = NTrim(p + 1); continue;
        }
        if (ai + 1 < MAX_STR_LEN) args[count][ai++] = *p;
        p++;
    }
    if (ai > 0 && count < max_args && *p == '\0') {
        args[count][ai] = '\0'; NRTrim(args[count]); count++;
    }
    return count;
}

static int EvalStringArg(const char *expr, char *out, uint32_t out_size, const char *label) {
    NyotaVal v = Eval(expr);
    if (v.type != TYPE_STR || !v.s[0]) {
        char err[128];
        NStrCopy(err, label, sizeof(err));
        NStrAppend(err, " wymaga niepustego STRING", sizeof(err));
        OutError(err);
        return 0;
    }
    NStrCopy(out, v.s, out_size);
    return 1;
}

static int ParenLooksLikeTuple(const char *expr) {
    const char *p;
    int depth = 0, in_str = 0;
    if (!expr || *expr != '(') return 0;
    p = NTrim(expr + 1);
    if (*p == ')') return 1;
    p = expr + 1;
    while (*p) {
        if (*p == '"') in_str = !in_str;
        else if (!in_str) {
            if (*p == '(' || *p == '[' || *p == '{') depth++;
            else if (*p == ')' || *p == ']' || *p == '}') {
                if (*p == ')' && depth == 0) return 0;
                if (depth > 0) depth--;
            } else if (*p == ',' && depth == 0) return 1;
        }
        p++;
    }
    return 0;
}

static NyotaVal ParseTupleLiteral(const char **pp) {
    NyotaVal result;
    const char *expr = NTrim(*pp);
    const char *p = NTrim(expr + 1);
    ValClear(&result);
    result.type = TYPE_TUPLE;
    if (*p == ')') { *pp = p + 1; return result; }
    while (*p) {
        char elem_buf[MAX_STR_LEN];
        uint32_t ei = 0;
        int depth = 0, in_str = 0;
        while (*p && ei + 1 < MAX_STR_LEN) {
            if (*p == '"') in_str = !in_str;
            else if (!in_str) {
                if (*p == '(' || *p == '[' || *p == '{') depth++;
                else if (*p == ')' || *p == ']' || *p == '}') {
                    if (*p == ')' && depth == 0) break;
                    if (depth > 0) depth--;
                } else if (*p == ',' && depth == 0) break;
            }
            elem_buf[ei++] = *p++;
        }
        elem_buf[ei] = '\0';
        NRTrim(elem_buf);
        if (!NTrim(elem_buf)[0]) {
            OutError("TUPLE: pusty element");
            ValClear(&result);
            *pp = p;
            return result;
        }
        if (!ListEnsureCapacity(&result, result.list_len + 1)) {
            ValClear(&result);
            *pp = p;
            return result;
        }
        result.list_items[result.list_len++] = Eval(elem_buf);
        p = NTrim(p);
        if (*p == ',') {
            p = NTrim(p + 1);
            if (*p == ')') { *pp = p + 1; return result; }
            continue;
        }
        if (*p == ')') { *pp = p + 1; return result; }
        OutError("TUPLE: oczekiwano , albo )");
        ValClear(&result);
        *pp = p;
        return result;
    }
    OutError("TUPLE: brak zamykajacego )");
    ValClear(&result);
    return result;
}

static int ParseTimeTriple(const char *p, int32_t *seconds) {
    int32_t hh, mm, ss;
    if (!p || !NIsDigit(p[0]) || !NIsDigit(p[1]) || p[2] != '.' ||
        !NIsDigit(p[3]) || !NIsDigit(p[4]) || p[5] != '.' ||
        !NIsDigit(p[6]) || !NIsDigit(p[7]) || p[8] != ')') return 0;
    hh = (p[0] - '0') * 10 + (p[1] - '0');
    mm = (p[3] - '0') * 10 + (p[4] - '0');
    ss = (p[6] - '0') * 10 + (p[7] - '0');
    if (hh > 23 || mm > 59 || ss > 59) return -1;
    *seconds = hh * 3600 + mm * 60 + ss;
    return 1;
}

static int ParseTimeShift(const char *p, int64_t *seconds, uint32_t *consumed) {
    char unit;
    uint32_t i = 1;
    int64_t value = 0;
    if (!p || (p[0] != 'H' && p[0] != 'M' && p[0] != 'S') || !NIsDigit(p[1])) return 0;
    unit = p[0];
    while (NIsDigit(p[i])) {
        value = value * 10 + (p[i] - '0');
        i++;
    }
    if (NIsAlpha(p[i]) || NIsDigit(p[i]) || p[i] == '_') return 0;
    if (unit == 'H') value *= 3600;
    else if (unit == 'M') value *= 60;
    *seconds = value;
    *consumed = i;
    return 1;
}

// Prosta ewaluacja wyrażenia (bez rekurencji dla nawiasów — linearny parser)
static NyotaVal ParsePrimary(const char **pp) {
    NyotaVal result;
    ValClear(&result);

    const char *expr = NTrim(*pp);
    const char *call_open = 0;
    {
        const char *q = expr;
        if (NIsAlpha(*q)) {
            while (*q && (NIsAlpha(*q) || NIsDigit(*q) || *q == '_')) q++;
            if (*q == '(') call_open = q;
        }
    }
    if (expr[0] == '(') {
        if (ParenLooksLikeTuple(expr)) return ParseTupleLiteral(pp);
        {
            const char *inner = expr + 1;
            result = ParseOr((const char **)&inner);
            *pp = MatchParen(expr);
            return result;
        }
    }

    // String literal
    if (expr[0] == '"') {
        uint32_t consumed = 0;
        ParseStringLit(expr, result.s, MAX_STR_LEN, &consumed);
        result.type = TYPE_STR;
        *pp = expr + consumed; return result;
    }

    // Lista [...]
    if (expr[0] == '[') {
        const char *p = NTrim(expr + 1);
        ValClear(&result);
        result.type = TYPE_LIST;
        while (*p && *p != ']') {
            char elem_buf[MAX_STR_LEN];
            uint32_t ei = 0;
            int depth = 0, in_str = 0;
            while (*p && ei + 1 < MAX_STR_LEN) {
                if (*p == '"') {
                    in_str = !in_str;
                } else if (!in_str) {
                    if (*p == '(' || *p == '[' || *p == '{') {
                        depth++;
                    } else if (*p == ')' || *p == ']' || *p == '}') {
                        if (*p == ']' && depth == 0) break;
                        if (depth > 0) depth--;
                    } else if (*p == ',' && depth == 0) {
                        break;
                    }
                }
                elem_buf[ei++] = *p++;
            }
            elem_buf[ei] = '\0';
            NRTrim(elem_buf);
            if (!NTrim(elem_buf)[0]) {
                OutError("LIST: pusty element");
                ValClear(&result);
                *pp = p;
                return result;
            }
            if (!ListEnsureCapacity(&result, result.list_len + 1)) {
                ValClear(&result);
                *pp = p;
                return result;
            }
            result.list_items[result.list_len++] = Eval(elem_buf);
            if (*p == ',') {
                p = NTrim(p + 1);
                if (*p == ']') {
                    OutError("LIST: pusty element po przecinku");
                    ValClear(&result);
                    *pp = p;
                    return result;
                }
                continue;
            }
            p = NTrim(p);
            if (*p != ']') {
                OutError("LIST: oczekiwano , albo ]");
                ValClear(&result);
                *pp = p;
                return result;
            }
        }
        if (*p != ']') {
            OutError("LIST: brak zamykajacego ]");
            ValClear(&result);
            *pp = p;
            return result;
        }
        *pp = p + 1;
        return result;
    }

    // Literał MARK: {N| key | v | v , key | v | v }
    if (expr[0] == '{') {
        const char *p = NTrim(expr + 1);
        uint32_t cons = 0;
        NyotaVal nf;
        ValClear(&nf);
        if (!ParseNumber(p, &nf, &cons) || nf.type != TYPE_INT || nf.i < 1) {
            OutError("MARK wymaga {N| ... } z N >= 1");
            *pp = expr;
            return result;
        }
        p = NTrim(p + cons);
        if (*p != '|') {
            OutError("MARK: brak | po liczbie kolumn");
            *pp = expr;
            return result;
        }
        p++;
        {
            int32_t total = nf.i;
            NyotaVal *rows = PoolAlloc(MAX_LIST_ITEMS);
            if (!rows) { OutError("Pula list pelna"); *pp = expr; return result; }
            ValClear(&result);
            result.type = TYPE_MARK;
            result.i = total - 1;
            result.list_items = rows;
            result.list_cap = MAX_LIST_ITEMS;
            result.list_len = 0;
            while (*p && *p != '}') {
                p = NTrim(p);
                if (*p == '}' || !*p) break;
                if (*p == ',') { p++; continue; }
                {
                    NyotaVal *fields = PoolAlloc((uint32_t)total);
                    NyotaVal row;
                    int32_t f;
                    if (!fields) { OutError("Pula list pelna"); *pp = expr; return result; }
                    ValClear(&row);
                    row.type = TYPE_LIST;
                    row.list_items = fields;
                    row.list_cap = (uint32_t)total;
                    row.list_len = 0;
                    for (f = 0; f < total; f++) {
                        char field[MAX_STR_LEN];
                        uint32_t ei = 0;
                        int depth = 0, in_str = 0;
                        p = NTrim(p);
                        while (*p && ei + 1 < MAX_STR_LEN) {
                            if (*p == '"') in_str = !in_str;
                            else if (!in_str && (*p == '(' || *p == '[' || *p == '{')) depth++;
                            else if (!in_str && (*p == ')' || *p == ']' || *p == '}')) {
                                if (depth == 0) break;
                                depth--;
                            } else if (!in_str && depth == 0 && (*p == '|' || *p == ','))
                                break;
                            field[ei++] = *p++;
                        }
                        field[ei] = '\0';
                        NRTrim(field);
                        row.list_items[row.list_len++] = Eval(field);
                        p = NTrim(p);
                        if (f + 1 < total) {
                            if (*p != '|') {
                                OutError("MARK: za malo pol w wierszu");
                                *pp = expr;
                                ValClear(&result);
                                return result;
                            }
                            p++;
                        }
                    }
                    if (FindMarkRow(&result, &row.list_items[0]) >= 0) {
                        OutError("MARK: duplikat klucza");
                        ValClear(&result);
                        *pp = expr;
                        return result;
                    }
                    if (result.list_len < result.list_cap)
                        result.list_items[result.list_len++] = row;
                }
            }
            if (*p == '}') p++;
            else {
                OutError("MARK: brak zamykajacego }");
                ValClear(&result);
                *pp = expr;
                return result;
            }
            *pp = p;
            return result;
        }
    }

    // Literał DATE: <RRRR.MM.DD>
    if (expr[0] == '<') {
        uint32_t cons = 0;
        int32_t y = 0, m = 0, d = 0;
        if (ParseDateLit(expr, &y, &m, &d, &cons)) {
            if (!DateValid(y, m, d)) {
                OutError("Niepoprawna data");
                ValClear(&result);
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            ValFromDateSerial(&result, DateToSerial(y, m, d));
            *pp = expr + cons; return result;
        }
        if (NIsDigit(expr[1])) {
            OutError("Niepoprawny literal DATE (wymagane <RRRR.MM.DD>)");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
    }

    // TRUE / FALSE
    if (NStartsWith(expr, "TRUE")) {
        ValFromBool(&result, 1);
        *pp = expr + 4; return result;
    }
    if (NStartsWith(expr, "FALSE")) {
        ValFromBool(&result, 0);
        *pp = expr + 5; return result;
    }

    // Nazwane kolory NyotaUI sa stalymi jezyka.
    if (NIsAlpha(expr[0])) {
        char color_name[32];
        uint32_t cn = ParseIdent(expr, color_name, sizeof(color_name));
        NyotaColor color;
        if (cn > 0 && NyotaColorParse(color_name, &color)) {
            ValFromColor(&result, &color);
            *pp = expr + cn;
            return result;
        }
    }

    // Funkcje wbudowane: INT(...), STRING(...), LEN(...)
    if (NStrEqN(expr, "INT(", 4)) {
        NyotaVal inner = Eval(expr + 4);
        if (inner.type == TYPE_DATE) {
            OutError("INT() nie konwertuje DATE (uzyj YEAR/MONTH/DAY)");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        ValFromInt(&result, ValToInt(&inner));
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "TODAY(", 6)) {
        const char *tp = NTrim(expr + 6);
        if (*tp != ')') {
            OutError("TODAY() nie przyjmuje argumentow");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        if (!g_host || !g_host->unix_time) {
            OutError("TODAY() wymaga czasu systemowego");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        {
            uint64_t ut = HostUnixTime();
            int32_t serial = DateToSerial(1970, 1, 1) + (int32_t)(ut / 86400ULL);
            ValFromDateSerial(&result, serial);
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "TIME(", 5)) {
        const char *tp = NTrim(expr + 5);
        int32_t sec = 0;
        if (*tp == ')') {
            ValFromTimeSeconds(&result, (int64_t)HostLocalTimeSeconds());
        } else {
            int ok = ParseTimeTriple(tp, &sec);
            if (ok == 0) {
                OutError("TIME wymaga TIME(HH.MM.SS)");
                ValClear(&result);
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            if (ok < 0) {
                OutError("TIME: zakres HH 00..23, MM/SS 00..59");
                ValClear(&result);
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            ValFromTimeSeconds(&result, sec);
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "HOUR(", 5) || NStrEqN(expr, "MINUTE(", 7) || NStrEqN(expr, "SECOND(", 7)) {
        const char *inner_s;
        int which;
        NyotaVal inner;
        if (NStrEqN(expr, "HOUR(", 5)) { inner_s = expr + 5; which = 0; }
        else if (NStrEqN(expr, "MINUTE(", 7)) { inner_s = expr + 7; which = 1; }
        else { inner_s = expr + 7; which = 2; }
        inner = Eval(inner_s);
        if (inner.type != TYPE_TIME) {
            OutError("HOUR/MINUTE/SECOND wymaga TIME");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        if (which == 0) ValFromInt(&result, inner.i / 3600);
        else if (which == 1) ValFromInt(&result, (inner.i / 60) % 60);
        else ValFromInt(&result, inner.i % 60);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "YEAR(", 5) || NStrEqN(expr, "MONTH(", 6) || NStrEqN(expr, "DAY(", 4)) {
        const char *inner_s = expr;
        int which = 0;
        if (NStrEqN(expr, "YEAR(", 5)) { inner_s = expr + 5; which = 0; }
        else if (NStrEqN(expr, "MONTH(", 6)) { inner_s = expr + 6; which = 1; }
        else { inner_s = expr + 4; which = 2; }
        {
            NyotaVal inner = Eval(inner_s);
            int32_t y = 0, m = 0, d = 0;
            if (inner.type != TYPE_DATE) {
                OutError("YEAR/MONTH/DAY wymaga DATE");
                ValClear(&result);
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            SerialToDate(inner.i, &y, &m, &d);
            if (which == 0) ValFromInt(&result, y);
            else if (which == 1) ValFromInt(&result, m);
            else ValFromInt(&result, d);
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "STRING(", 7) || NStrEqN(expr, "STR(", 4)) {
        const char *inner_s = NStrEqN(expr, "STRING(", 7) ? expr + 7 : expr + 4;
        NyotaVal inner = Eval(inner_s);
        ValFromStr(&result, "");
        ValToStr(&inner, result.s, MAX_STR_LEN);
        result.type = TYPE_STR;
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "FLT(", 4)) {
        NyotaVal inner = Eval(expr + 4);
        if (inner.type == TYPE_FLOAT) return inner;
        if (inner.type == TYPE_INT) { ValFromFloat(&result, inner.i, 0); return result; }
        if (inner.type == TYPE_STR) {
            uint32_t cons = 0;
            if (ParseNumber(NTrim(inner.s), &result, &cons)) return result;
        }
        OutError("FLT() wymaga INTEGER, FLOAT lub STRING liczbowy");
        ValClear(&result);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "BOOL(", 5)) {
        NyotaVal inner = Eval(expr + 5);
        if (inner.type == TYPE_BOOL) return inner;
        if (inner.type == TYPE_INT) { ValFromBool(&result, inner.i != 0); return result; }
        if (inner.type == TYPE_FLOAT) {
            ValFromBool(&result, inner.f_int != 0 || inner.f_frac != 0);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        OutError("BOOL() wymaga INTEGER, FLOAT lub BOOLEAN");
        ValClear(&result);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "TUPLE(", 6) || NStrEqN(expr, "LIST(", 5)) {
        char args[1][MAX_STR_LEN];
        int is_tuple = NStrEqN(expr, "TUPLE(", 6);
        int n = SplitFunctionArgs(expr + (is_tuple ? 6 : 5), args, 1);
        NyotaVal inner;
        if (n != 1) {
            OutError(is_tuple ? "TUPLE() wymaga jednego argumentu" : "LIST() wymaga jednego argumentu");
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        inner = Eval(args[0]);
        if (inner.type != TYPE_LIST && inner.type != TYPE_TUPLE) {
            OutError(is_tuple ? "TUPLE() wymaga LIST albo TUPLE" : "LIST() wymaga TUPLE albo LIST");
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        result = SeqCloneAs(&inner, is_tuple ? TYPE_TUPLE : TYPE_LIST);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "FILE_READ(", 10)) {
        char args[1][MAX_STR_LEN], path[MAX_STR_LEN];
        uint32_t got = 0;
        int n = SplitFunctionArgs(expr + 10, args, 1);
        int rc;
        if (n != 1 || !EvalStringArg(args[0], path, sizeof(path), "FILE_READ()")) {
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        if (!g_host || !g_host->file_read) {
            OutError("FILE_READ: host nie obsluguje plikow");
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        ValFromStr(&result, "");
        rc = HostFileRead(path, result.s, MAX_STR_LEN, &got);
        if (rc == -2) OutError("FILE_READ: plik jest za duzy dla STRING (max 511 bajtow)");
        else if (rc == -3) OutError("FILE_READ: plik binarny z NUL nie jest obslugiwany");
        else if (rc != 0) OutError("FILE_READ: nie mozna odczytac pliku");
        if (rc != 0) ValClear(&result);
        else result.type = TYPE_STR;
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "FILE_EXISTS(", 12) || NStrEqN(expr, "DIR_EXISTS(", 11)) {
        char args[1][MAX_STR_LEN], path[MAX_STR_LEN];
        int is_dir = NStrEqN(expr, "DIR_EXISTS(", 11);
        int off = is_dir ? 11 : 12;
        int n = SplitFunctionArgs(expr + off, args, 1);
        if (n != 1 || !EvalStringArg(args[0], path, sizeof(path), is_dir ? "DIR_EXISTS()" : "FILE_EXISTS()")) {
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        ValFromBool(&result, is_dir ? HostDirExists(path) : HostFileExists(path));
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "FILE_SIZE(", 10)) {
        char args[1][MAX_STR_LEN], path[MAX_STR_LEN];
        int64_t size;
        int n = SplitFunctionArgs(expr + 10, args, 1);
        if (n != 1 || !EvalStringArg(args[0], path, sizeof(path), "FILE_SIZE()")) {
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        size = HostFileSize(path);
        if (size < 0) {
            OutError("FILE_SIZE: nie mozna pobrac rozmiaru");
            ValClear(&result);
        } else if (size > INT32_MAX) {
            OutError("FILE_SIZE: rozmiar przekracza INTEGER Nyoty");
            ValClear(&result);
        } else ValFromInt(&result, (int32_t)size);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "DIR_LIST(", 9) || NStrEqN(expr, "LS(", 3)) {
        char args[1][MAX_STR_LEN], path[MAX_STR_LEN];
        int off = NStrEqN(expr, "DIR_LIST(", 9) ? 9 : 3;
        uint32_t got = 0, pos = 0;
        int n = SplitFunctionArgs(expr + off, args, 1);
        int rc;
        if (n != 1 || !EvalStringArg(args[0], path, sizeof(path), off == 9 ? "DIR_LIST()" : "LS()")) {
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        if (!g_host || !g_host->dir_list) {
            OutError("DIR_LIST/LS: host nie obsluguje katalogow");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        rc = HostDirList(path, g_dir_list_buf, sizeof(g_dir_list_buf), &got);
        if (rc == -2) OutError("DIR_LIST: wynik przekracza bufor 8192 bajtow");
        else if (rc != 0) OutError("DIR_LIST: nie mozna odczytac katalogu");
        if (rc != 0) {
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        ValClear(&result);
        result.type = TYPE_LIST;
        while (pos < got) {
            char name[MAX_STR_LEN];
            uint32_t ni = 0;
            while (pos < got && g_dir_list_buf[pos] != '\n') {
                if (ni + 1 >= sizeof(name)) {
                    OutError("DIR_LIST: nazwa wpisu jest za dluga");
                    ValClear(&result);
                    *pp = call_open ? MatchParen(call_open) : expr;
                    return result;
                }
                name[ni++] = g_dir_list_buf[pos++];
            }
            if (pos < got && g_dir_list_buf[pos] == '\n') pos++;
            name[ni] = '\0';
            if (!ListEnsureCapacity(&result, result.list_len + 1)) {
                ValClear(&result);
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            ValFromStr(&result.list_items[result.list_len++], name);
        }
        ListSortAlgo(&result, 0, LIST_SORT_AUTO);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }

    if (NStrEqN(expr, "SPRITE_X(", 9) || NStrEqN(expr, "SPRITE_Y(", 9) ||
        NStrEqN(expr, "SPRITE_W(", 9) || NStrEqN(expr, "SPRITE_H(", 9) ||
        NStrEqN(expr, "SPRITE_VISIBLE(", 15)) {
        char args[1][MAX_STR_LEN];
        char sname[64];
        int n, which = 0, off = 9;
        NyotaSprite *s;
        if (NStrEqN(expr, "SPRITE_Y(", 9)) which = 1;
        else if (NStrEqN(expr, "SPRITE_W(", 9)) which = 2;
        else if (NStrEqN(expr, "SPRITE_H(", 9)) which = 3;
        else if (NStrEqN(expr, "SPRITE_VISIBLE(", 15)) { which = 4; off = 15; }
        n = SplitFunctionArgs(expr + off, args, 1);
        if (n != 1 || !SpriteArgName(args[0], sname, sizeof(sname))) {
            OutError("SPRITE_X/Y/W/H/VISIBLE wymaga jednej nazwy SPRITE");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        s = FindSprite(sname);
        if (!s) {
            OutError("Nieznany SPRITE");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        if (which == 0) ValFromInt(&result, s->x);
        else if (which == 1) ValFromInt(&result, s->y);
        else if (which == 2) ValFromInt(&result, s->w);
        else if (which == 3) ValFromInt(&result, s->h);
        else ValFromBool(&result, s->visible != 0);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "SPRITE_FRAME(", 13) ||
        NStrEqN(expr, "SPRITE_PLAYING(", 15)) {
        char args[1][MAX_STR_LEN];
        char sname[64];
        int is_playing = NStrEqN(expr, "SPRITE_PLAYING(", 15);
        int off = is_playing ? 15 : 13;
        int n = SplitFunctionArgs(expr + off, args, 1);
        NyotaSprite *s;
        if (n != 1 || !SpriteArgName(args[0], sname, sizeof(sname))) {
            OutError("SPRITE_FRAME/PLAYING wymaga jednej nazwy SPRITE");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        s = FindSprite(sname);
        if (!s) {
            OutError("Nieznany SPRITE");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        SpriteUpdateAnimation(s);
        if (is_playing) ValFromBool(&result, s->playing != 0);
        else ValFromInt(&result, (int32_t)s->frame);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "SPRITE_HIT(", 11)) {
        char args[2][MAX_STR_LEN];
        char aname[64], bname[64];
        NyotaSprite *a, *b;
        int n = SplitFunctionArgs(expr + 11, args, 2);
        if (n != 2 ||
            !SpriteArgName(args[0], aname, sizeof(aname)) ||
            !SpriteArgName(args[1], bname, sizeof(bname))) {
            OutError("SPRITE_HIT() wymaga dwoch nazw SPRITE");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        a = FindSprite(aname);
        b = FindSprite(bname);
        if (!a || !b) {
            OutError("SPRITE_HIT: nieznany SPRITE");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        ValFromBool(&result, SpriteHit(a, b));
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "BUTTON_CLICKED(", 15)) {
        char args[2][MAX_STR_LEN];
        char bname[64];
        int n = SplitFunctionArgs(expr + 15, args, 2);
        NyotaButton *b;
        int clicked;
        if (n != 1) {
            OutError("BUTTON_CLICKED() wymaga jednej nazwy BUTTON");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        {
            const char *a = NTrim(args[0]);
            if (*a == '"') {
                NyotaVal nv = Eval(a);
                if (nv.type != TYPE_STR || !nv.s[0]) {
                    OutError("BUTTON_CLICKED() wymaga nazwy BUTTON");
                    ValClear(&result);
                    *pp = call_open ? MatchParen(call_open) : expr;
                    return result;
                }
                NStrCopy(bname, nv.s, sizeof(bname));
            } else {
                uint32_t bn = ParseIdent(a, bname, sizeof(bname));
                if (!bname[0] || *NTrim(a + bn)) {
                    OutError("BUTTON_CLICKED() wymaga identyfikatora albo STRING");
                    ValClear(&result);
                    *pp = call_open ? MatchParen(call_open) : expr;
                    return result;
                }
            }
        }
        {
            NyotaUiControl *uc = FindUiControl(bname);
            if (uc && uc->spec.kind == NYOTA_UI_CTRL_BUTTON) {
                clicked = HostUiButtonClicked(uc->host_handle);
                if (clicked < 0) {
                    OutError("BUTTON_CLICKED: host nie obsluguje zdarzen BUTTON");
                    ValClear(&result);
                    *pp = call_open ? MatchParen(call_open) : expr;
                    return result;
                }
                ValFromBool(&result, clicked != 0);
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
        }
        b = FindButton(bname);
        if (!b) {
            OutError("BUTTON_CLICKED: nieznany BUTTON");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        clicked = ButtonPollClicked(b);
        if (clicked < 0) {
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        ValFromBool(&result, clicked);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "CBOX_CHECKED(", 13) || NStrEqN(expr, "RADIO_CHECKED(", 14)) {
        int radio = NStrEqN(expr, "RADIO_CHECKED(", 14);
        int off = radio ? 14 : 13; char args[2][MAX_STR_LEN], name[64]; int n=SplitFunctionArgs(expr+off,args,2); NyotaUiControl *uc; int32_t v;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError(radio?"RADIO_CHECKED() wymaga nazwy RADIO":"CBOX_CHECKED() wymaga nazwy CBOX");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(radio?NYOTA_UI_CTRL_RADIO:NYOTA_UI_CTRL_CBOX)){OutError("CHECKED: nieznana kontrolka");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        v=HostUiControlChecked(uc->host_handle);if(v<0){OutError("CHECKED: host nie obsluguje stanu");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}uc->spec.checked=(uint8_t)(v!=0);ValFromBool(&result,v!=0);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "CBOX_SET(", 9) || NStrEqN(expr, "RADIO_SET(", 10)) {
        int radio=NStrEqN(expr,"RADIO_SET(",10);int off=radio?10:9;char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+off,args,3);NyotaUiControl*uc;NyotaVal bv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("CBOX_SET/RADIO_SET wymaga (nazwa, BOOLEAN)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}bv=Eval(args[1]);if(bv.type!=TYPE_BOOL){OutError("CBOX_SET/RADIO_SET: drugi argument wymaga BOOLEAN");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(radio?NYOTA_UI_CTRL_RADIO:NYOTA_UI_CTRL_CBOX)||HostUiControlSetChecked(uc->host_handle,(uint8_t)(bv.i!=0))!=0){OutError("CBOX_SET/RADIO_SET: nie mozna ustawic stanu");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}uc->spec.checked=(uint8_t)(bv.i!=0);ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "COMBO_INDEX(", 12) || NStrEqN(expr, "COMBO_VALUE(", 12)) {
        int value=NStrEqN(expr,"COMBO_VALUE(",12);char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+12,args,2);NyotaUiControl*uc;int32_t ix;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("COMBO_INDEX/VALUE wymaga nazwy COMBO");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_COMBO){OutError("COMBO: nieznana kontrolka");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}ix=HostUiComboIndex(uc->host_handle);if(ix<0){OutError("COMBO: host nie zwrocil wyboru");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}uc->spec.selected=(uint32_t)ix;
        if(!value)ValFromInt(&result,ix);else{uint32_t cur=0,start=0,i=0;ValClear(&result);result.type=TYPE_STR;result.s[0]='\0';while(1){char ch=uc->spec.items[i];if(ch=='\n'||ch=='\0'){if(cur==(uint32_t)ix){uint32_t len=i-start;if(len>=sizeof(result.s))len=sizeof(result.s)-1;memcpy(result.s,uc->spec.items+start,len);result.s[len]='\0';break;}cur++;start=i+1;if(ch=='\0')break;}i++;}}
        *pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "COMBO_SET(", 10)) {
        char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+10,args,3);NyotaUiControl*uc;NyotaVal iv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("COMBO_SET wymaga (nazwa, indeks)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}iv=Eval(args[1]);if(iv.type!=TYPE_INT||iv.i<0){OutError("COMBO_SET: indeks wymaga nieujemnego INTEGER");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_COMBO||(uint32_t)iv.i>=UiItemsCount(uc->spec.items)||HostUiComboSetIndex(uc->host_handle,(uint32_t)iv.i)!=0){OutError("COMBO_SET: indeks poza zakresem albo host odrzucil");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}uc->spec.selected=(uint32_t)iv.i;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "TAREA_TEXT(", 11)) {
        char args[2][MAX_STR_LEN],name[64],buf[NYOTA_UI_TEXT_MAX];uint32_t got=0;
        int n=SplitFunctionArgs(expr+11,args,2);NyotaUiControl*uc;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("TAREA_TEXT() wymaga nazwy TAREA");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_TAREA){OutError("TAREA_TEXT: nieznana TAREA");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        if(HostUiTareaGetText(uc->host_handle,buf,sizeof(buf),&got)!=0){OutError("TAREA_TEXT: host nie zwrocil tekstu");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValClear(&result);result.type=TYPE_STR;NStrCopy(result.s,buf,sizeof(result.s));NStrCopy(uc->spec.text,buf,sizeof(uc->spec.text));*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "TAREA_SET(", 10)) {
        char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+10,args,3);NyotaUiControl*uc;NyotaVal tv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("TAREA_SET wymaga (nazwa, STRING)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        tv=Eval(args[1]);if(tv.type!=TYPE_STR){OutError("TAREA_SET: drugi argument wymaga STRING");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_TAREA||HostUiTareaSetText(uc->host_handle,tv.s)!=0){OutError("TAREA_SET: host odrzucil tekst");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        NStrCopy(uc->spec.text,tv.s,sizeof(uc->spec.text));ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "TAREA_CHANGED(", 14)) {
        char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+14,args,2);NyotaUiControl*uc;int32_t ch;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("TAREA_CHANGED() wymaga nazwy TAREA");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_TAREA){OutError("TAREA_CHANGED: nieznana TAREA");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ch=HostUiTareaChanged(uc->host_handle);if(ch<0){OutError("TAREA_CHANGED: host nie obsluguje zdarzenia");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValFromBool(&result,ch!=0);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SBAR_VALUE(", 11) || NStrEqN(expr, "PBAR_VALUE(", 11)) {
        int pbar=NStrEqN(expr,"PBAR_VALUE(",11);char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+11,args,2);NyotaUiControl*uc;int32_t v;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError(pbar?"PBAR_VALUE() wymaga nazwy PBAR":"SBAR_VALUE() wymaga nazwy SBAR");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(pbar?NYOTA_UI_CTRL_PBAR:NYOTA_UI_CTRL_SBAR)){OutError("VALUE: nieznana kontrolka SBAR/PBAR");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        v=HostUiRangeValue(uc->host_handle);if(v<0){OutError("VALUE: host nie zwrocil wartosci");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.range_value=(uint32_t)v;ValFromInt(&result,v);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SBAR_SET(", 9) || NStrEqN(expr, "PBAR_SET(", 9)) {
        int pbar=NStrEqN(expr,"PBAR_SET(",9);char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+9,args,3);NyotaUiControl*uc;NyotaVal vv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("SBAR_SET/PBAR_SET wymaga (nazwa, INTEGER)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        vv=Eval(args[1]);if(vv.type!=TYPE_INT||vv.i<0){OutError("SBAR_SET/PBAR_SET: wartosc wymaga nieujemnego INTEGER");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(pbar?NYOTA_UI_CTRL_PBAR:NYOTA_UI_CTRL_SBAR)||(uint32_t)vv.i<uc->spec.range_min||(uint32_t)vv.i>uc->spec.range_max||HostUiRangeSetValue(uc->host_handle,(uint32_t)vv.i)!=0){OutError("SBAR_SET/PBAR_SET: wartosc poza zakresem albo host odrzucil");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.range_value=(uint32_t)vv.i;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SLIDER_VALUE(", 13)) {
        char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+13,args,2);NyotaUiControl*uc;int32_t v;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("SLIDER_VALUE() wymaga nazwy SLIDER");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_SLIDER){OutError("SLIDER_VALUE: nieznany SLIDER");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        v=HostUiRangeValue(uc->host_handle);if(v<0){OutError("SLIDER_VALUE: host nie zwrocil wartosci");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.range_value=(uint32_t)v;ValFromInt(&result,v);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SLIDER_SET(", 11)) {
        char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+11,args,3);NyotaUiControl*uc;NyotaVal vv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("SLIDER_SET wymaga (nazwa, INTEGER)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        vv=Eval(args[1]);if(vv.type!=TYPE_INT||vv.i<0){OutError("SLIDER_SET: wartosc wymaga nieujemnego INTEGER");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_SLIDER||(uint32_t)vv.i<uc->spec.range_min||(uint32_t)vv.i>uc->spec.range_max||HostUiRangeSetValue(uc->host_handle,(uint32_t)vv.i)!=0){OutError("SLIDER_SET: wartosc poza zakresem albo host odrzucil");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.range_value=(uint32_t)vv.i;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "EQBOX_SET(", 10)) {
        char args[3][MAX_STR_LEN],name[64];uint32_t vals[NYOTA_UI_EQ_MAX_BARS],i;int n=SplitFunctionArgs(expr+10,args,3);NyotaUiControl*uc;NyotaVal lv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("EQBOX_SET wymaga (nazwa, LIST)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_EQBOX){OutError("EQBOX_SET: nieznany EQBOX");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        lv=Eval(args[1]);if(lv.type!=TYPE_LIST||lv.list_len!=uc->spec.eq_bars){OutError("EQBOX_SET: LIST musi miec dokladnie BARS elementow");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        for(i=0;i<lv.list_len;i++){if(lv.list_items[i].type!=TYPE_INT||lv.list_items[i].i<0||(uint32_t)lv.list_items[i].i<uc->spec.range_min||(uint32_t)lv.list_items[i].i>uc->spec.range_max){OutError("EQBOX_SET: wartosci musza byc INTEGER w zakresie MIN..MAX");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}vals[i]=(uint32_t)lv.list_items[i].i;}
        if(HostUiEqboxSetValues(uc->host_handle,vals,uc->spec.eq_bars)!=0){OutError("EQBOX_SET: host odrzucil wartosci");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        for(i=0;i<uc->spec.eq_bars;i++)uc->spec.eq_values[i]=vals[i];uc->spec.eq_value_count=uc->spec.eq_bars;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "EQBOX_BAR(", 10)) {
        char args[4][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+10,args,4);NyotaUiControl*uc;NyotaVal iv,vv;
        if(n!=3||!SpriteArgName(args[0],name,sizeof(name))){OutError("EQBOX_BAR wymaga (nazwa, indeks, wartosc)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);iv=Eval(args[1]);vv=Eval(args[2]);
        if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_EQBOX||iv.type!=TYPE_INT||vv.type!=TYPE_INT||iv.i<0||(uint32_t)iv.i>=uc->spec.eq_bars||vv.i<0||(uint32_t)vv.i<uc->spec.range_min||(uint32_t)vv.i>uc->spec.range_max||HostUiEqboxSetBar(uc->host_handle,(uint32_t)iv.i,(uint32_t)vv.i)!=0){OutError("EQBOX_BAR: indeks/wartosc poza zakresem albo host odrzucil");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.eq_values[iv.i]=(uint32_t)vv.i;if(uc->spec.eq_value_count<=(uint32_t)iv.i)uc->spec.eq_value_count=(uint32_t)iv.i+1;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "TBOX_TEXT(", 10)) {
        char args[2][MAX_STR_LEN],name[64],buf[NYOTA_UI_TEXT_MAX];uint32_t got=0;int n=SplitFunctionArgs(expr+10,args,2);NyotaUiControl*uc;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("TBOX_TEXT() wymaga nazwy TBOX");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_TBOX||HostUiTareaGetText(uc->host_handle,buf,sizeof(buf),&got)!=0){OutError("TBOX_TEXT: host nie zwrocil tekstu");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValClear(&result);result.type=TYPE_STR;NStrCopy(result.s,buf,sizeof(result.s));NStrCopy(uc->spec.text,buf,sizeof(uc->spec.text));*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "TBOX_SET(", 9)) {
        char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+9,args,3);NyotaUiControl*uc;NyotaVal tv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("TBOX_SET wymaga (nazwa, STRING)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        tv=Eval(args[1]);uc=FindUiControl(name);
        if(tv.type!=TYPE_STR||!uc||uc->spec.kind!=NYOTA_UI_CTRL_TBOX||strlen(tv.s)>uc->spec.max_length||strchr(tv.s,'\n')||strchr(tv.s,'\r')||HostUiTareaSetText(uc->host_handle,tv.s)!=0){OutError("TBOX_SET: nieprawidlowy tekst albo host odrzucil");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        NStrCopy(uc->spec.text,tv.s,sizeof(uc->spec.text));ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "TBOX_CHANGED(", 13)) {
        char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+13,args,2);NyotaUiControl*uc;int32_t ch;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("TBOX_CHANGED() wymaga nazwy TBOX");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_TBOX||(ch=HostUiTareaChanged(uc->host_handle))<0){OutError("TBOX_CHANGED: host nie obsluguje zdarzenia");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValFromBool(&result,ch!=0);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SPINBOX_VALUE(", 14) || NStrEqN(expr, "SCALE_VALUE(", 12)) {
        int scale=NStrEqN(expr,"SCALE_VALUE(",12);int off=scale?12:14;char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+off,args,2);NyotaUiControl*uc;int32_t v;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("SPINBOX_VALUE/SCALE_VALUE wymaga nazwy kontrolki");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(scale?NYOTA_UI_CTRL_SCALE:NYOTA_UI_CTRL_SPINBOX)||HostUiSignedValue(uc->host_handle,&v)!=0){OutError("signed VALUE: host nie zwrocil wartosci");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.signed_value=v;ValFromInt(&result,v);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SPINBOX_SET(", 12) || NStrEqN(expr, "SCALE_SET(", 10)) {
        int scale=NStrEqN(expr,"SCALE_SET(",10);int off=scale?10:12;char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+off,args,3);NyotaUiControl*uc;NyotaVal vv;int32_t v;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("SPINBOX_SET/SCALE_SET wymaga (nazwa, INTEGER)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        vv=Eval(args[1]);uc=FindUiControl(name);
        if(vv.type!=TYPE_INT||!uc||uc->spec.kind!=(scale?NYOTA_UI_CTRL_SCALE:NYOTA_UI_CTRL_SPINBOX)){OutError("SPINBOX_SET/SCALE_SET: zle argumenty");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        v=vv.i;if(scale&&uc->spec.scale_wrap)v=UiWrapSigned(v,uc->spec.signed_min,uc->spec.signed_max);
        if(v<uc->spec.signed_min||v>uc->spec.signed_max||(scale&&uc->spec.scale_wrap&&v>=uc->spec.signed_max)||HostUiSignedSetValue(uc->host_handle,v)!=0){OutError("SPINBOX_SET/SCALE_SET: wartosc poza zakresem albo host odrzucil");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.signed_value=v;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SPINBOX_CHANGED(", 16) || NStrEqN(expr, "SCALE_CHANGED(", 14)) {
        int scale=NStrEqN(expr,"SCALE_CHANGED(",14);int off=scale?14:16;char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+off,args,2);NyotaUiControl*uc;int32_t ch;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("SPINBOX_CHANGED/SCALE_CHANGED wymaga nazwy kontrolki");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(scale?NYOTA_UI_CTRL_SCALE:NYOTA_UI_CTRL_SPINBOX)||(ch=HostUiControlChanged(uc->host_handle))<0){OutError("CHANGED: host nie obsluguje kontrolki");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValFromBool(&result,ch!=0);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SPLITTER_VALUE(", 15)) {
        char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+15,args,2);NyotaUiControl*uc;int32_t v;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("SPLITTER_VALUE() wymaga nazwy SPLITTER");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_SPLITTER||(v=HostUiRangeValue(uc->host_handle))<0){OutError("SPLITTER_VALUE: host nie zwrocil wartosci");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.range_value=(uint32_t)v;ValFromInt(&result,v);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SPLITTER_SET(", 13)) {
        char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+13,args,3);NyotaUiControl*uc;NyotaVal vv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("SPLITTER_SET wymaga (nazwa, INTEGER)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        vv=Eval(args[1]);uc=FindUiControl(name);
        if(vv.type!=TYPE_INT||vv.i<0||!uc||uc->spec.kind!=NYOTA_UI_CTRL_SPLITTER||(uint32_t)vv.i<uc->spec.range_min||(uint32_t)vv.i>uc->spec.range_max||HostUiRangeSetValue(uc->host_handle,(uint32_t)vv.i)!=0){OutError("SPLITTER_SET: wartosc poza zakresem albo host odrzucil");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.range_value=(uint32_t)vv.i;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "SPLITTER_CHANGED(", 17)) {
        char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+17,args,2);NyotaUiControl*uc;int32_t ch;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("SPLITTER_CHANGED() wymaga nazwy SPLITTER");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_SPLITTER||(ch=HostUiControlChanged(uc->host_handle))<0){OutError("SPLITTER_CHANGED: host nie obsluguje zdarzenia");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValFromBool(&result,ch!=0);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "LISTVIEW_INDEX(", 15) || NStrEqN(expr, "TREEVIEW_INDEX(", 15)) {
        int tree=NStrEqN(expr,"TREEVIEW_INDEX(",15);char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+15,args,2);NyotaUiControl*uc;int32_t ix;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("LISTVIEW_INDEX/TREEVIEW_INDEX wymaga nazwy");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(tree?NYOTA_UI_CTRL_TREEVIEW:NYOTA_UI_CTRL_LISTVIEW)||(ix=HostUiSelectIndex(uc->host_handle))<0){OutError("INDEX: host nie zwrocil zaznaczenia");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.selected=(uint32_t)ix;ValFromInt(&result,ix);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "LISTVIEW_SET(", 13) || NStrEqN(expr, "TREEVIEW_SET(", 13)) {
        int tree=NStrEqN(expr,"TREEVIEW_SET(",13);char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+13,args,3);NyotaUiControl*uc;NyotaVal iv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("LISTVIEW_SET/TREEVIEW_SET wymaga (nazwa, indeks)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        iv=Eval(args[1]);uc=FindUiControl(name);
        if(iv.type!=TYPE_INT||iv.i<0||!uc||uc->spec.kind!=(tree?NYOTA_UI_CTRL_TREEVIEW:NYOTA_UI_CTRL_LISTVIEW)||(uint32_t)iv.i>=UiItemsCount(uc->spec.items)||HostUiSelectSetIndex(uc->host_handle,(uint32_t)iv.i)!=0){OutError("LISTVIEW_SET/TREEVIEW_SET: indeks poza zakresem");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.selected=(uint32_t)iv.i;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "LISTVIEW_VALUE(", 15) || NStrEqN(expr, "TREEVIEW_VALUE(", 15)) {
        int tree=NStrEqN(expr,"TREEVIEW_VALUE(",15);char args[2][MAX_STR_LEN],name[64],item[NYOTA_UI_TEXT_MAX];int n=SplitFunctionArgs(expr+15,args,2);NyotaUiControl*uc;int32_t ix;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("LISTVIEW_VALUE/TREEVIEW_VALUE wymaga nazwy");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(tree?NYOTA_UI_CTRL_TREEVIEW:NYOTA_UI_CTRL_LISTVIEW)||(ix=HostUiSelectIndex(uc->host_handle))<0||!UiItemText(uc->spec.items,(uint32_t)ix,item,sizeof(item))){OutError("VALUE: brak zaznaczonego elementu");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValClear(&result);result.type=TYPE_STR;NStrCopy(result.s,item,sizeof(result.s));*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "LISTVIEW_CHANGED(", 17) || NStrEqN(expr, "TREEVIEW_CHANGED(", 17)) {
        int tree=NStrEqN(expr,"TREEVIEW_CHANGED(",17);char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+17,args,2);NyotaUiControl*uc;int32_t ch;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("LISTVIEW_CHANGED/TREEVIEW_CHANGED wymaga nazwy");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=(tree?NYOTA_UI_CTRL_TREEVIEW:NYOTA_UI_CTRL_LISTVIEW)||(ch=HostUiControlChanged(uc->host_handle))<0){OutError("CHANGED: host nie obsluguje listy/drzewa");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValFromBool(&result,ch!=0);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "CLOCK_SET(", 10)) {
        char args[3][MAX_STR_LEN],name[64];int32_t vals[NYOTA_UI_CLOCK_MAX_NEEDLES];uint32_t i;int n=SplitFunctionArgs(expr+10,args,3);NyotaUiControl*uc;NyotaVal lv;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("CLOCK_SET wymaga (nazwa, LIST)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);lv=Eval(args[1]);
        if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_CLOCK||lv.type!=TYPE_LIST||lv.list_len!=uc->spec.clock_needles){OutError("CLOCK_SET: LIST musi miec dokladnie NEEDLES elementow");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        for(i=0;i<lv.list_len;i++){if(lv.list_items[i].type!=TYPE_INT||lv.list_items[i].i<uc->spec.clock_needle_min[i]||lv.list_items[i].i>uc->spec.clock_needle_max[i]){OutError("CLOCK_SET: wartosc poza zakresem wskazowki");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}vals[i]=lv.list_items[i].i;}
        if(HostUiClockSetValues(uc->host_handle,vals,uc->spec.clock_needles)!=0){OutError("CLOCK_SET: host odrzucil wartosci");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        memcpy(uc->spec.clock_values,vals,uc->spec.clock_needles*sizeof(int32_t));uc->spec.clock_value_count=uc->spec.clock_needles;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "CLOCK_NEEDLE(", 13)) {
        char args[4][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+13,args,4);NyotaUiControl*uc;NyotaVal iv,vv;
        if(n!=3||!SpriteArgName(args[0],name,sizeof(name))){OutError("CLOCK_NEEDLE wymaga (nazwa, indeks, wartosc)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);iv=Eval(args[1]);vv=Eval(args[2]);
        if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_CLOCK||iv.type!=TYPE_INT||vv.type!=TYPE_INT||iv.i<0||(uint32_t)iv.i>=uc->spec.clock_needles||vv.i<uc->spec.clock_needle_min[iv.i]||vv.i>uc->spec.clock_needle_max[iv.i]||HostUiClockSetNeedle(uc->host_handle,(uint32_t)iv.i,vv.i)!=0){OutError("CLOCK_NEEDLE: indeks/wartosc poza zakresem");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc->spec.clock_values[iv.i]=vv.i;if(uc->spec.clock_value_count<=(uint32_t)iv.i)uc->spec.clock_value_count=(uint32_t)iv.i+1;ValFromBool(&result,1);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "CLOCK_VALUE(", 12)) {
        char args[3][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+12,args,3);NyotaUiControl*uc;NyotaVal iv;int32_t v;
        if(n!=2||!SpriteArgName(args[0],name,sizeof(name))){OutError("CLOCK_VALUE wymaga (nazwa, indeks)");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);iv=Eval(args[1]);
        if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_CLOCK||iv.type!=TYPE_INT||iv.i<0||(uint32_t)iv.i>=uc->spec.clock_needles||HostUiClockValue(uc->host_handle,(uint32_t)iv.i,&v)!=0){OutError("CLOCK_VALUE: indeks poza zakresem");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        ValFromInt(&result,v);*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "CLOCK_VALUES(", 13)) {
        char args[2][MAX_STR_LEN],name[64];int n=SplitFunctionArgs(expr+13,args,2);NyotaUiControl*uc;NyotaVal *items;uint32_t i;
        if(n!=1||!SpriteArgName(args[0],name,sizeof(name))){OutError("CLOCK_VALUES() wymaga nazwy CLOCK");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        uc=FindUiControl(name);if(!uc||uc->spec.kind!=NYOTA_UI_CTRL_CLOCK){OutError("CLOCK_VALUES: nieznany CLOCK");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        items=PoolAlloc(uc->spec.clock_needles);if(!items){OutError("Pula list pelna");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}
        for(i=0;i<uc->spec.clock_needles;i++){int32_t v;if(HostUiClockValue(uc->host_handle,i,&v)!=0){OutError("CLOCK_VALUES: host nie zwrocil wartosci");ValClear(&result);*pp=call_open?MatchParen(call_open):expr;return result;}ValFromInt(&items[i],v);}
        ValClear(&result);result.type=TYPE_LIST;result.list_items=items;result.list_len=uc->spec.clock_needles;result.list_cap=uc->spec.clock_needles;*pp=call_open?MatchParen(call_open):expr;return result;
    }
    if (NStrEqN(expr, "DAREA_DROPPED(", 14)) {
        char args[2][MAX_STR_LEN], name[64];
        int n = SplitFunctionArgs(expr + 14, args, 2);
        NyotaUiControl *uc;
        int32_t dropped;
        if (n != 1 || !SpriteArgName(args[0], name, sizeof(name))) {
            OutError("DAREA_DROPPED() wymaga nazwy DAREA");
            ValClear(&result); *pp = call_open ? MatchParen(call_open) : expr; return result;
        }
        uc = FindUiControl(name);
        if (!uc || uc->spec.kind != NYOTA_UI_CTRL_DAREA) {
            OutError("DAREA_DROPPED: nieznana DAREA");
            ValClear(&result); *pp = call_open ? MatchParen(call_open) : expr; return result;
        }
        dropped = HostUiDareaDropped(uc->host_handle);
        if (dropped < 0) {
            OutError("DAREA_DROPPED: host nie obsluguje drop");
            ValClear(&result); *pp = call_open ? MatchParen(call_open) : expr; return result;
        }
        ValFromBool(&result, dropped != 0);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "DAREA_ITEMS(", 12)) {
        char args[2][MAX_STR_LEN], name[64], data[NYOTA_UI_DROP_MAX];
        uint32_t out_size = 0, count = 0, i = 0, start = 0;
        int n = SplitFunctionArgs(expr + 12, args, 2);
        NyotaUiControl *uc;
        NyotaVal *items;
        if (n != 1 || !SpriteArgName(args[0], name, sizeof(name))) {
            OutError("DAREA_ITEMS() wymaga nazwy DAREA");
            ValClear(&result); *pp = call_open ? MatchParen(call_open) : expr; return result;
        }
        uc = FindUiControl(name);
        if (!uc || uc->spec.kind != NYOTA_UI_CTRL_DAREA) {
            OutError("DAREA_ITEMS: nieznana DAREA");
            ValClear(&result); *pp = call_open ? MatchParen(call_open) : expr; return result;
        }
        if (HostUiDareaItems(uc->host_handle, data, sizeof(data), &out_size) != 0) {
            OutError("DAREA_ITEMS: host nie zwrocil elementow");
            ValClear(&result); *pp = call_open ? MatchParen(call_open) : expr; return result;
        }
        if (out_size >= sizeof(data)) out_size = sizeof(data) - 1;
        data[out_size] = '\0';
        for (i = 0; i < out_size; i++) if (data[i] == '\n') count++;
        if (out_size && data[out_size - 1] != '\n') count++;
        if (count > MAX_LIST_ITEMS) {
            OutError("DAREA_ITEMS: za duzo elementow");
            ValClear(&result); *pp = call_open ? MatchParen(call_open) : expr; return result;
        }
        items = count ? PoolAlloc(count) : 0;
        if (count && !items) {
            OutError("Pula list pelna");
            ValClear(&result); *pp = call_open ? MatchParen(call_open) : expr; return result;
        }
        count = 0; start = 0;
        for (i = 0; i <= out_size; i++) {
            if (i == out_size || data[i] == '\n') {
                if (i > start) {
                    uint32_t len = i - start;
                    ValClear(&items[count]);
                    items[count].type = TYPE_STR;
                    if (len >= sizeof(items[count].s)) len = sizeof(items[count].s) - 1;
                    memcpy(items[count].s, data + start, len);
                    items[count].s[len] = '\0';
                    count++;
                }
                start = i + 1;
            }
        }
        ValClear(&result);
        result.type = TYPE_LIST; result.list_items = items; result.list_len = count; result.list_cap = count;
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "LEN(", 4)) {
        NyotaVal inner = Eval(expr + 4);
        int32_t len = 0;
        if (inner.type == TYPE_STR) len = (int32_t)NStrLen(inner.s);
        else if (inner.type == TYPE_LIST || inner.type == TYPE_TUPLE || inner.type == TYPE_MARK)
            len = (int32_t)inner.list_len;
        else {
            OutError("LEN() wymaga STRING, LIST, TUPLE albo MARK");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        ValFromInt(&result, len);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "MINFO(", 6)) {
        NyotaVal inner = Eval(expr + 6);
        if (inner.type != TYPE_MARK) {
            OutError("MINFO() wymaga MARK");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        {
            NyotaVal *items = PoolAlloc(2);
            if (!items) { OutError("Pula list pelna"); ValClear(&result); *pp = MatchParen(call_open); return result; }
            ValFromInt(&items[0], (int32_t)inner.list_len);
            ValFromInt(&items[1], inner.i);
            ValClear(&result);
            result.type = TYPE_LIST;
            result.list_items = items;
            result.list_len = 2;
            result.list_cap = 2;
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "KEY(", 4)) {
        char args[2][MAX_STR_LEN];
        int n = SplitFunctionArgs(expr + 4, args, 2);
        if (n != 2) { OutError("KEY(mark, wiersz)"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        {
            NyotaVal mk = Eval(args[0]);
            NyotaVal ix = Eval(args[1]);
            int32_t i;
            if (mk.type != TYPE_MARK) OutError("KEY() wymaga MARK");
            else if (ix.type != TYPE_INT) OutError("KEY: indeks wiersza wymaga INTEGER");
            else {
                i = ix.i;
                if (i < 0 || (uint32_t)i >= mk.list_len) OutError("KEY: wiersz poza zakresem");
                else if (mk.list_items[i].type == TYPE_LIST && mk.list_items[i].list_len > 0)
                    result = mk.list_items[i].list_items[0];
            }
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "VALUE(", 6)) {
        char args[3][MAX_STR_LEN];
        int n = SplitFunctionArgs(expr + 6, args, 3);
        if (n != 3) { OutError("VALUE(mark, wiersz, kolumna)"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        {
            NyotaVal mk = Eval(args[0]);
            NyotaVal rv = Eval(args[1]);
            NyotaVal cv = Eval(args[2]);
            int32_t r, c;
            if (mk.type != TYPE_MARK) OutError("VALUE() wymaga MARK");
            else if (rv.type != TYPE_INT || cv.type != TYPE_INT) OutError("VALUE: wiersz i kolumna wymagaja INTEGER");
            else {
                r = rv.i; c = cv.i;
                if (r < 0 || (uint32_t)r >= mk.list_len) OutError("VALUE: wiersz poza zakresem");
                else if (c < 0 || c >= mk.i) OutError("VALUE: kolumna poza zakresem");
                else if (mk.list_items[r].type == TYPE_LIST
                         && (uint32_t)(c + 1) < mk.list_items[r].list_len)
                    result = mk.list_items[r].list_items[c + 1];
            }
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "VALUES(", 7)) {
        static char args[MAX_LIST_ITEMS + 2][MAX_STR_LEN];
        int n = SplitFunctionArgs(expr + 7, args, MAX_LIST_ITEMS + 2);
        int ai;
        NyotaVal mk, rv;
        int32_t row;
        if (n < 3) { OutError("VALUES(mark, wiersz, kolumna/range...)"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        mk = Eval(args[0]); rv = Eval(args[1]);
        if (mk.type != TYPE_MARK) { OutError("VALUES() wymaga MARK"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        if (rv.type != TYPE_INT) { OutError("VALUES: indeks wiersza wymaga INTEGER"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        row = rv.i;
        if (row < 0 || (uint32_t)row >= mk.list_len) { OutError("VALUES: wiersz poza zakresem"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        ValClear(&result); result.type = TYPE_LIST;
        for (ai = 2; ai < n; ai++) {
            char *sel = (char *)NTrim(args[ai]);
            int32_t colon = NFind(sel, ":");
            int32_t from, to, c;
            if (colon >= 0) {
                char left[128], right[128];
                uint32_t k, sl = NStrLen(sel);
                NyotaVal lv, hv;
                if ((uint32_t)colon >= sizeof(left)) { OutError("VALUES: zly zakres"); ValClear(&result); break; }
                for (k = 0; k < (uint32_t)colon; k++) left[k] = sel[k];
                left[colon] = '\0';
                NStrCopy(right, sel + colon + 1, sizeof(right));
                NRTrim(left); NRTrim(right);
                (void)sl;
                lv = Eval(NTrim(left)); hv = Eval(NTrim(right));
                if (lv.type != TYPE_INT || hv.type != TYPE_INT) { OutError("VALUES: zakres wymaga INTEGER"); ValClear(&result); break; }
                from = lv.i; to = hv.i;
                if (from > to) { OutError("VALUES: zakres musi rosnac"); ValClear(&result); break; }
            } else {
                NyotaVal cv = Eval(sel);
                if (cv.type != TYPE_INT) { OutError("VALUES: kolumna wymaga INTEGER"); ValClear(&result); break; }
                from = to = cv.i;
            }
            if (from < 0 || to >= mk.i) { OutError("VALUES: kolumna poza zakresem"); ValClear(&result); break; }
            for (c = from; c <= to; c++) {
                if (!ListEnsureCapacity(&result, result.list_len + 1)) { ValClear(&result); break; }
                result.list_items[result.list_len++] = mk.list_items[row].list_items[c + 1];
            }
            if (result.type == TYPE_NONE) break;
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "MLIST(", 6)) {
        char args[2][MAX_STR_LEN];
        int n = SplitFunctionArgs(expr + 6, args, 2);
        NyotaVal mk;
        uint32_t i;
        int key_mode = 0;
        int32_t col = -1;
        if (n != 2) { OutError("MLIST(mark, KEY/kolumna)"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        mk = Eval(args[0]);
        if (mk.type != TYPE_MARK) { OutError("MLIST() wymaga MARK"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        if (NStrEq(NTrim(args[1]), "KEY")) key_mode = 1;
        else {
            NyotaVal cv = Eval(args[1]);
            if (cv.type != TYPE_INT) { OutError("MLIST: kolumna wymaga INTEGER albo KEY"); ValClear(&result); *pp = MatchParen(call_open); return result; }
            col = cv.i;
            if (col < 0 || col >= mk.i) { OutError("MLIST: kolumna poza zakresem"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        }
        ValClear(&result); result.type = TYPE_LIST;
        if (mk.list_len && !ListEnsureCapacity(&result, mk.list_len)) { ValClear(&result); *pp = MatchParen(call_open); return result; }
        for (i = 0; i < mk.list_len; i++)
            result.list_items[result.list_len++] = mk.list_items[i].list_items[key_mode ? 0 : col + 1];
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "SUM(", 4) || NStrEqN(expr, "AVG(", 4) || NStrEqN(expr, "MED(", 4) ||
        NStrEqN(expr, "MIN(", 4) || NStrEqN(expr, "MAX(", 4) || NStrEqN(expr, "MODE(", 5) ||
        NStrEqN(expr, "MODECOUNT(", 10) || NStrEqN(expr, "COUNT(", 6)) {
        const char *ap = expr;
        int off = 4, kind = -1, expected = 2;
        char args[3][MAX_STR_LEN];
        int n;
        NyotaVal mk, cv, needle;
        int32_t col;
        ValClear(&needle);
        if (NStrEqN(expr, "SUM(", 4)) { kind = 0; off = 4; }
        else if (NStrEqN(expr, "AVG(", 4)) { kind = 1; off = 4; }
        else if (NStrEqN(expr, "MED(", 4)) { kind = 2; off = 4; }
        else if (NStrEqN(expr, "MIN(", 4)) { kind = 3; off = 4; }
        else if (NStrEqN(expr, "MAX(", 4)) { kind = 4; off = 4; }
        else if (NStrEqN(expr, "MODE(", 5)) { kind = 5; off = 5; }
        else if (NStrEqN(expr, "MODECOUNT(", 10)) { kind = 6; off = 10; }
        else { kind = 7; off = 6; expected = 3; }
        n = SplitFunctionArgs(ap + off, args, 3);
        if (n != expected) { OutError(kind == 7 ? "COUNT(mark, kolumna, wartosc)" : "Statystyka MARK wymaga (mark, kolumna)"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        mk = Eval(args[0]); cv = Eval(args[1]);
        if (mk.type != TYPE_MARK) { OutError("Statystyka wymaga MARK"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        if (cv.type != TYPE_INT) { OutError("Statystyka MARK: kolumna wymaga INTEGER"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        col = cv.i;
        if (kind == 7) needle = Eval(args[2]);
        MarkStat(&mk, col, kind, kind == 7 ? &needle : 0, &result);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "SIN(", 4)) {
        NyotaVal inner = Eval(expr + 4);
        ValFromInt(&result, NSin(ValToInt(&inner)));
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "COS(", 4)) {
        NyotaVal inner = Eval(expr + 4);
        ValFromInt(&result, NCos(ValToInt(&inner)));
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "UPPER(", 6)) {
        NyotaVal inner = Eval(expr + 6);
        ValFromStr(&result, inner.type == TYPE_STR ? inner.s : "");
        NToUpper(result.s);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "LOWER(", 6)) {
        NyotaVal inner = Eval(expr + 6);
        ValFromStr(&result, inner.type == TYPE_STR ? inner.s : "");
        NToLower(result.s);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "FIND(", 5)) {
        // FIND(haystack, needle) - szukaj podciagu
        const char *p = expr + 5;
        // Parsuj dwa argumenty rozdzielone przecinkiem
        char arg1[MAX_STR_LEN], arg2[MAX_STR_LEN];
        uint32_t ai = 0; int depth = 0; int in_str = 0;
        while (*p && ai + 1 < MAX_STR_LEN) {
            if (*p == '"') in_str = !in_str;
            if (!in_str && *p == '(') depth++;
            if (!in_str && *p == ')') { if (depth == 0) break; depth--; }
            if (!in_str && depth == 0 && *p == ',') break;
            arg1[ai++] = *p++;
        }
        arg1[ai] = '\0'; NRTrim(arg1);
        if (*p == ',') p = NTrim(p + 1);
        ai = 0; depth = 0; in_str = 0;
        while (*p && *p != ')' && ai + 1 < MAX_STR_LEN) {
            if (*p == '"') in_str = !in_str;
            if (!in_str && *p == '(') depth++;
            if (!in_str && *p == ')') { if (depth == 0) break; depth--; }
            arg2[ai++] = *p++;
        }
        arg2[ai] = '\0'; NRTrim(arg2);
        NyotaVal v1 = Eval(arg1);
        NyotaVal v2 = Eval(arg2);
        char s1[MAX_STR_LEN], s2[MAX_STR_LEN];
        ValToStr(&v1, s1, sizeof(s1));
        ValToStr(&v2, s2, sizeof(s2));
        ValFromInt(&result, NFind(s1, s2));
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "FLOOR(", 6)) {
        NyotaVal inner = Eval(expr + 6);
        if (inner.type == TYPE_FLOAT) {
            int32_t v = inner.f_int;
            if (inner.f_frac < 0 && inner.f_int >= 0) v--;
            else if (inner.f_int < 0 && inner.f_frac != 0) v--;
            ValFromInt(&result, v);
        } else { ValFromInt(&result, ValToInt(&inner)); }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "CEIL(", 5)) {
        NyotaVal inner = Eval(expr + 5);
        if (inner.type == TYPE_FLOAT) {
            int32_t v = inner.f_int;
            if (inner.f_frac > 0) v++;
            ValFromInt(&result, v);
        } else { ValFromInt(&result, ValToInt(&inner)); }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "ROUND(", 6)) {
        NyotaVal inner = Eval(expr + 6);
        if (inner.type == TYPE_FLOAT) {
            int32_t f = inner.f_frac; if (f < 0) f = -f;
            int32_t v = inner.f_int;
            if (f >= 500) { if (inner.f_int >= 0) v++; else v--; }
            ValFromInt(&result, v);
        } else { ValFromInt(&result, ValToInt(&inner)); }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "RANDINT(", 8)) {
        char args[3][MAX_STR_LEN];
        int n = SplitFunctionArgs(expr + 8, args, 3);
        if (n != 3) {
            OutError("RANDINT(count, min, max)");
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        {
            NyotaVal cv = Eval(args[0]);
            NyotaVal minv = Eval(args[1]);
            NyotaVal maxv = Eval(args[2]);
            int32_t count, min, max;
            uint32_t i;
            if (cv.type != TYPE_INT || minv.type != TYPE_INT || maxv.type != TYPE_INT) {
                OutError("RANDINT wymaga argumentow INTEGER");
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            count = cv.i; min = minv.i; max = maxv.i;
            if (count < 0 || count > MAX_LIST_ITEMS || min > max) {
                OutError("RANDINT: zly count albo zakres");
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            ValClear(&result);
            result.type = TYPE_LIST;
            if (count > 0 && !ListEnsureCapacity(&result, (uint32_t)count)) {
                ValClear(&result);
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            for (i = 0; i < (uint32_t)count; i++) {
                uint64_t span = (uint64_t)((int64_t)max - (int64_t)min) + 1ULL;
                uint64_t rnd;
                g_rng = g_rng * 1664525U + 1013904223U;
                rnd = (uint64_t)g_rng;
                ValFromInt(&result.list_items[result.list_len++],
                           (int32_t)((int64_t)min + (int64_t)(rnd % span)));
            }
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "RANDFLT(", 8)) {
        char args[4][MAX_STR_LEN];
        int n = SplitFunctionArgs(expr + 8, args, 4);
        if (n != 3 && n != 4) {
            OutError("RANDFLT(count, min, max [, precision])");
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        {
            NyotaVal cv = Eval(args[0]);
            NyotaVal minv = Eval(args[1]);
            NyotaVal maxv = Eval(args[2]);
            int32_t count, precision = 2, step;
            int64_t minm, maxm, lo, hi, span;
            uint32_t i;
            if (cv.type != TYPE_INT ||
                (minv.type != TYPE_INT && minv.type != TYPE_FLOAT) ||
                (maxv.type != TYPE_INT && maxv.type != TYPE_FLOAT)) {
                OutError("RANDFLT wymaga count INTEGER i granic liczbowych");
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            if (n == 4) {
                NyotaVal pv = Eval(args[3]);
                if (pv.type != TYPE_INT) {
                    OutError("RANDFLT: precision wymaga INTEGER");
                    *pp = call_open ? MatchParen(call_open) : expr;
                    return result;
                }
                precision = pv.i;
            }
            count = cv.i;
            if (count < 0 || count > MAX_LIST_ITEMS || precision < 0 || precision > 3) {
                OutError("RANDFLT: count 0..64, precision 0..3");
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            minm = (minv.type == TYPE_FLOAT) ? FloatMilli(&minv) : (int64_t)minv.i * 1000;
            maxm = (maxv.type == TYPE_FLOAT) ? FloatMilli(&maxv) : (int64_t)maxv.i * 1000;
            if (minm > maxm) {
                OutError("RANDFLT: minimum wieksze od maksimum");
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            step = precision == 0 ? 1000 : (precision == 1 ? 100 : (precision == 2 ? 10 : 1));
            lo = minm >= 0 ? (minm + step - 1) / step : -((-minm) / step);
            hi = maxm >= 0 ? maxm / step : -(((-maxm) + step - 1) / step);
            if (lo > hi) {
                OutError("RANDFLT: brak wartosci w zakresie dla tej precyzji");
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            span = hi - lo + 1;
            ValClear(&result);
            result.type = TYPE_LIST;
            if (count > 0 && !ListEnsureCapacity(&result, (uint32_t)count)) {
                ValClear(&result);
                *pp = call_open ? MatchParen(call_open) : expr;
                return result;
            }
            for (i = 0; i < (uint32_t)count; i++) {
                int64_t units, milli;
                g_rng = g_rng * 1664525U + 1013904223U;
                units = lo + (int64_t)((uint64_t)g_rng % (uint64_t)span);
                milli = units * step;
                ValFromFloat(&result.list_items[result.list_len++], 0, (int32_t)milli);
            }
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "RANDOM(", 7)) {
        NyotaVal inner = Eval(expr + 7);
        int32_t max = ValToInt(&inner);
        g_rng = g_rng * 1664525U + 1013904223U;
        int32_t r = (int32_t)((g_rng >> 16) & 0x7FFF);
        if (max > 0) r = r % max;
        ValFromInt(&result, r);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "SIMILARITY(", 11)) {
        char args[2][MAX_STR_LEN];
        if (SplitFunctionArgs(expr + 11, args, 2) == 2) {
            NyotaVal v1 = Eval(args[0]); NyotaVal v2 = Eval(args[1]);
            char s1[MAX_STR_LEN], s2[MAX_STR_LEN];
            ValToStr(&v1, s1, sizeof(s1)); ValToStr(&v2, s2, sizeof(s2));
            ValFromInt(&result, Levenshtein(s1, s2));
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "FUZZY(", 6)) {
        char args[3][MAX_STR_LEN];
        if (SplitFunctionArgs(expr + 6, args, 3) == 3) {
            NyotaVal v0 = Eval(args[0]);
            int32_t val = ValToInt(&v0);
            NyotaVal v1 = Eval(args[1]);
            int32_t min = ValToInt(&v1);
            NyotaVal v2 = Eval(args[2]);
            int32_t max = ValToInt(&v2);
            if (val <= min) ValFromInt(&result, 0);
            else if (val >= max) ValFromInt(&result, 100);
            else ValFromInt(&result, (val - min) * 100 / (max - min));
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "PATHFIND(", 9)) {
        char args[4][MAX_STR_LEN];
        if (SplitFunctionArgs(expr + 9, args, 4) == 4) {
            NyotaVal v0 = Eval(args[0]); NyotaVal v1 = Eval(args[1]);
            int32_t sx = ValToInt(&v0), sy = ValToInt(&v1);
            NyotaVal v2 = Eval(args[2]); NyotaVal v3 = Eval(args[3]);
            int32_t tx = ValToInt(&v2), ty = ValToInt(&v3);
            int32_t dx = tx - sx, dy = ty - sy;
            int32_t abs_dx = dx > 0 ? dx : -dx, abs_dy = dy > 0 ? dy : -dy;
            if (abs_dx > abs_dy) ValFromInt(&result, dx > 0 ? 1 : 3);
            else ValFromInt(&result, dy > 0 ? 2 : 0);
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "NN_PREDICT(", 11)) {
        char args[2][MAX_STR_LEN];
        if (SplitFunctionArgs(expr + 11, args, 2) == 2) {
            NyotaVal v0 = Eval(args[0]);
            int32_t id = ValToInt(&v0);
            NyotaVal lst = Eval(args[1]);
            if (id >= 0 && id < MAX_NN && g_nn[id].active && lst.type == TYPE_LIST) {
                int32_t sum = g_nn[id].bias;
                uint32_t in_cnt = g_nn[id].inputs > lst.list_len ? lst.list_len : g_nn[id].inputs;
                for (uint32_t i=0; i<in_cnt; i++) {
                    sum += (ValToInt(&lst.list_items[i]) * g_nn[id].weights[i]) / 1000;
                }
                if (sum < 0) sum = 0;
                if (sum > 1000) sum = 1000;
                ValFromInt(&result, sum);
            }
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "TOKENS(", 7)) {
        NyotaVal inner = Eval(expr + 7);
        result.type = TYPE_LIST;
        result.list_items = (g_list_pool_used + MAX_LIST_ITEMS <= LIST_POOL_SIZE) ? &g_list_pool[g_list_pool_used] : 0;
        result.list_cap = result.list_items ? MAX_LIST_ITEMS : 0;
        result.list_len = 0;
        if (result.list_items) g_list_pool_used += MAX_LIST_ITEMS;
        if (inner.type == TYPE_STR) {
            char *p = inner.s;
            while (*p) {
                while (*p && NIsSpace(*p)) p++;
                if (!*p) break;
                char buf[MAX_STR_LEN]; int bi = 0;
                while (*p && !NIsSpace(*p) && bi + 1 < MAX_STR_LEN) buf[bi++] = *p++;
                buf[bi] = '\0';
                if (result.list_len < MAX_LIST_ITEMS) ValFromStr(&result.list_items[result.list_len++], buf);
            }
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "MATCH(", 6)) {
        char args[2][MAX_STR_LEN];
        if (SplitFunctionArgs(expr + 6, args, 2) == 2) {
            char s1[MAX_STR_LEN], s2[MAX_STR_LEN];
            NyotaVal v0 = Eval(args[0]);
            ValToStr(&v0, s1, sizeof(s1));
            NyotaVal v1 = Eval(args[1]);
            ValToStr(&v1, s2, sizeof(s2));
            NToLower(s1); NToLower(s2);
            ValFromBool(&result, NMatchWildcard(s2, s1));
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "CHOOSE(", 7)) {
        NyotaVal lst = Eval(expr + 7);
        if (lst.type == TYPE_LIST && lst.list_len > 0) {
            g_rng = g_rng * 1664525U + 1013904223U;
            return lst.list_items[((g_rng >> 16) & 0x7FFF) % lst.list_len];
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "WEIGHTED(", 9)) {
        char args[2][MAX_STR_LEN];
        if (SplitFunctionArgs(expr + 9, args, 2) == 2) {
            NyotaVal items = Eval(args[0]); NyotaVal weights = Eval(args[1]);
            if (items.type == TYPE_LIST && weights.type == TYPE_LIST && items.list_len == weights.list_len && items.list_len > 0) {
                int32_t sum = 0;
                for (uint32_t i=0; i<weights.list_len; i++) sum += ValToInt(&weights.list_items[i]);
                if (sum > 0) {
                    g_rng = g_rng * 1664525U + 1013904223U;
                    int32_t r = (int32_t)(((g_rng >> 16) & 0x7FFF) % sum);
                    for (uint32_t i=0; i<items.list_len; i++) {
                        r -= ValToInt(&weights.list_items[i]);
                        if (r < 0) return items.list_items[i];
                    }
                }
            }
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "STATE_GET(", 10)) {
        NyotaVal v0 = Eval(expr + 10);
        int32_t id = ValToInt(&v0);
        ValFromStr(&result, "");
        if (id >= 0 && id < MAX_STATES && g_states_active[id]) ValFromStr(&result, g_states[id]);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "SIMPLIFY(", 9)) {
        NyotaVal inner = Eval(expr + 9);
        ValFromStr(&result, inner.type == TYPE_STR ? inner.s : "");
        NSimplify(result.s);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "NEAREST(", 8)) {
        char args[3][MAX_STR_LEN];
        if (SplitFunctionArgs(expr + 8, args, 3) == 3) {
            NyotaVal v0 = Eval(args[0]); NyotaVal v1 = Eval(args[1]);
            int32_t px = ValToInt(&v0), py = ValToInt(&v1);
            NyotaVal lst = Eval(args[2]);
            int32_t best_idx = -1; uint32_t best_dist = 0xFFFFFFFF;
            if (lst.type == TYPE_LIST) {
                for (uint32_t i=0; i<lst.list_len; i++) {
                    if (lst.list_items[i].type == TYPE_LIST && lst.list_items[i].list_len >= 2) {
                        int32_t dx = ValToInt(&lst.list_items[i].list_items[0]) - px;
                        int32_t dy = ValToInt(&lst.list_items[i].list_items[1]) - py;
                        uint32_t dist = (uint32_t)(dx*dx + dy*dy);
                        if (dist < best_dist) { best_dist = dist; best_idx = (int32_t)i; }
                    }
                }
            }
            ValFromInt(&result, best_idx);
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "VISIBLE(", 8)) {
        char args[5][MAX_STR_LEN];
        if (SplitFunctionArgs(expr + 8, args, 5) == 5) {
            NyotaVal v0 = Eval(args[0]); NyotaVal v1 = Eval(args[1]);
            int32_t x0 = ValToInt(&v0), y0 = ValToInt(&v1);
            NyotaVal v2 = Eval(args[2]); NyotaVal v3 = Eval(args[3]);
            int32_t x1 = ValToInt(&v2), y1 = ValToInt(&v3);
            NyotaVal rects = Eval(args[4]); int vis = 1;
            if (rects.type == TYPE_LIST) {
                int dx = x1 - x0; if (dx < 0) dx = -dx; int sx = x0 < x1 ? 1 : -1;
                int dy = y1 - y0; if (dy < 0) dy = -dy; int sy = y0 < y1 ? 1 : -1;
                int err = (dx > dy ? dx : -dy) / 2, e2;
                while (vis) {
                    for (uint32_t i=0; i<rects.list_len; i++) {
                        if (rects.list_items[i].type == TYPE_LIST && rects.list_items[i].list_len >= 4) {
                            int32_t rx = ValToInt(&rects.list_items[i].list_items[0]), ry = ValToInt(&rects.list_items[i].list_items[1]);
                            int32_t rw = ValToInt(&rects.list_items[i].list_items[2]), rh = ValToInt(&rects.list_items[i].list_items[3]);
                            if (x0 >= rx && x0 < rx+rw && y0 >= ry && y0 < ry+rh) { vis = 0; break; }
                        }
                    }
                    if (x0 == x1 && y0 == y1) break;
                    e2 = err; if (e2 > -dx) { err -= dy; x0 += sx; } if (e2 < dy) { err += dx; y0 += sy; }
                }
            }
            ValFromBool(&result, vis);
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
    if (NStrEqN(expr, "INPUT(", 6)) {
        // Pobierz prompt
        char prompt_str[MAX_STR_LEN] = "";
        const char *promptp = NTrim(expr + 6);
        if (*promptp == '"') {
            uint32_t consumed = 0;
            ParseStringLit(promptp, prompt_str, sizeof(prompt_str), &consumed);
        }
        // Wypisz prompt
        OutPrint(prompt_str);
        // Prosta linia wejściowa
        char input_buf[256]; uint32_t input_len = 0;
        input_buf[0] = '\0';
        // Rysuj kursor wejściowy i czekaj na znaki
        while (1) {
            // Narysuj bieżący bufor
            HostRect(g_out_x, g_out_y, SCREEN_W - g_out_x, OUT_FONT_H, 10, 10, 20);
            if (input_len > 0)
                HostText(g_out_x, g_out_y, input_buf, 220, 220, 220, 2);
            uint8_t key = HostWaitKey();
            uint8_t mods = HostKeyMods();
            if (key == 0x1C) break;  // Enter
            if (key == 0x0E && input_len > 0) { input_buf[--input_len] = '\0'; continue; }
            // Konwertuj scancode → char (uproszczona mapa)
            char c = 0;
            if (key >= 0x10 && key <= 0x19) {
                const char r[] = "qwertyuiop";
                c = (mods & 1) ? (char)(r[key-0x10]-32) : r[key-0x10];
            } else if (key >= 0x1E && key <= 0x26) {
                const char r[] = "asdfghjkl";
                c = (mods & 1) ? (char)(r[key-0x1E]-32) : r[key-0x1E];
            } else if (key >= 0x2C && key <= 0x32) {
                const char r[] = "zxcvbnm";
                c = (mods & 1) ? (char)(r[key-0x2C]-32) : r[key-0x2C];
            } else if (key >= 0x02 && key <= 0x0B) {
                const char n[] = "1234567890";
                c = n[key-0x02];
            } else if (key == 0x39) c = ' ';
            else if (key == 0x34) c = '.';
            else if (key == 0x0C) c = '-';
            if (c && input_len + 1 < sizeof(input_buf)) {
                input_buf[input_len++] = c;
                input_buf[input_len] = '\0';
            }
        }
        OutNewLine();
        ValFromStr(&result, input_buf);
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }

    // Kolor 0xRRGGBB / 0xRRGGBBAA
    if (expr[0] == '0' && (expr[1] == 'x' || expr[1] == 'X')) {
        char color_token[16];
        uint32_t n = 2;
        NyotaColor color;
        while (n < 10 && ((expr[n] >= '0' && expr[n] <= '9') ||
                          (expr[n] >= 'A' && expr[n] <= 'F') ||
                          (expr[n] >= 'a' && expr[n] <= 'f'))) n++;
        if ((n == 8 || n == 10) &&
            !((expr[n] >= '0' && expr[n] <= '9') ||
              (expr[n] >= 'A' && expr[n] <= 'F') ||
              (expr[n] >= 'a' && expr[n] <= 'f') ||
              NIsAlpha(expr[n]))) {
            uint32_t k;
            for (k = 0; k < n; k++) color_token[k] = expr[k];
            color_token[n] = '\0';
            if (NyotaColorParse(color_token, &color)) {
                ValFromColor(&result, &color);
                *pp = expr + n;
                return result;
            }
        }
        OutError("Niepoprawny kolor szesnastkowy (wymagane 0xRRGGBB albo 0xRRGGBBAA)");
        *pp = expr;
        return result;
    }

    // Liczba całkowita lub float
    if (NIsDigit(expr[0]) || (expr[0] == '-' && NIsDigit(expr[1]))) {
        uint32_t consumed = 0;
        if (!ParseNumber(expr, &result, &consumed)) {
            OutError("Niepoprawna liczba");
            ValClear(&result);
            *pp = call_open ? MatchParen(call_open) : expr;
            return result;
        }
        *pp = expr + consumed; return result;
    }

    // Identyfikator (zmienna lub wywołanie procedury)
    if (NIsAlpha(expr[0])) {
        char name[64];
        uint32_t nlen = ParseIdent(expr, name, sizeof(name));
        const char *after = NTrim(expr + nlen);

        // Wywołanie procedury/funkcji z nawiasami
        if (*after == '(') {
            const char *rest = after;
            NyotaVal v = CallNamed(name, after, &rest);
            *pp = rest; return v;
        }

        {
            NyotaVal *with_field = WithFieldPtr(name);
            NyotaVar *vv = 0;
            if (NStrEq(name, "ERR_CODE")) {
                ValFromInt(&result, g_err_code);
                *pp = after;
                return result;
            }
            if (with_field) {
                result = *with_field;
                *pp = after;
                return result;
            }
            vv = FindVar(name);
            if (!vv) {
                char err[128];
                NStrCopy(err, "Zmienna niezadeklarowana: ", sizeof(err));
                NStrAppend(err, name, sizeof(err));
                OutError(err);
                ValClear(&result);
                *pp = expr;
                return result;
            }
            result = vv->val;
            while (*after == '.') {
                char field[64];
                uint32_t fn;
                int idx;
                after = NTrim(after + 1);
                fn = ParseIdent(after, field, sizeof(field));
                if (!field[0]) {
                    OutError("RECORD: brak nazwy pola po .");
                    ValClear(&result);
                    *pp = after;
                    return result;
                }
                idx = RecordFieldIndex(&result, field);
                if (idx < 0) {
                    OutError("RECORD: nieznane pole");
                    ValClear(&result);
                    *pp = after + fn;
                    return result;
                }
                result = result.list_items[idx];
                after = NTrim(after + fn);
            }
            while (*after == '[') {
                char idx_buf[128];
                uint32_t si = 0;
                int depth = 0, in_str = 0;
                after++;
                while (*after && si + 1 < sizeof(idx_buf)) {
                    if (*after == '"') in_str = !in_str;
                    else if (!in_str && *after == '[') depth++;
                    else if (!in_str && *after == ']') {
                        if (depth == 0) break;
                        depth--;
                    }
                    idx_buf[si++] = *after++;
                }
                idx_buf[si] = '\0';
                if (*after == ']') after++;
                after = NTrim(after);
                if (result.type == TYPE_STR) {
                    int32_t colon_pos = -1;
                    uint32_t ci;
                    for (ci = 0; idx_buf[ci]; ci++) {
                        if (idx_buf[ci] == ':') { colon_pos = (int32_t)ci; break; }
                    }
                    if (colon_pos >= 0) {
                        uint32_t slen = NStrLen(result.s);
                        idx_buf[colon_pos] = '\0';
                        {
                            NyotaVal sv = Eval(idx_buf);
                            NyotaVal ev = Eval(idx_buf + colon_pos + 1);
                            int32_t start = ValToInt(&sv);
                            int32_t endv = ValToInt(&ev);
                            char tmp[MAX_STR_LEN];
                            uint32_t ti = 0;
                            if (start < 0) start = 0;
                            if ((uint32_t)endv > slen) endv = (int32_t)slen;
                            for (; start < endv && ti + 1 < MAX_STR_LEN; start++)
                                tmp[ti++] = result.s[start];
                            tmp[ti] = '\0';
                            ValFromStr(&result, tmp);
                        }
                        continue;
                    }
                }
                result = IndexValue(result, Eval(idx_buf));
            }
            *pp = after;
            return result;
        }
    }

    OutError("Niepoprawne wyrazenie");
    ValClear(&result);
    *pp = call_open ? MatchParen(call_open) : expr;
    return result;
}

static int PeekWord(const char *p, const char *w) {
    uint32_t n = NStrLen(w);
    if (!NStrEqN(p, w, n)) return 0;
    char c = p[n];
    return !NIsAlpha(c) && !NIsDigit(c) && c != '_';
}

static NyotaVal ValNeg(NyotaVal v) {
    if (v.type == TYPE_INT) { ValFromInt(&v, -v.i); return v; }
    if (v.type == TYPE_FLOAT) { ValFromFloat(&v, -v.f_int, -v.f_frac); return v; }
    OutError("Minus unarny wymaga liczby");
    ValClear(&v);
    return v;
}

static NyotaVal ParseUnary(const char **pp);
static NyotaVal ParsePostfix(const char **pp);
static NyotaVal ParsePower(const char **pp);
static NyotaVal ParseMul(const char **pp);
static NyotaVal ParseAdd(const char **pp);
static NyotaVal ParseShift(const char **pp);
static NyotaVal ParseBand(const char **pp);
static NyotaVal ParseBxor(const char **pp);
static NyotaVal ParseBor(const char **pp);
static NyotaVal ParseCompare(const char **pp);
static NyotaVal ParseNot(const char **pp);
static NyotaVal ParseAnd(const char **pp);

static NyotaVal ValFactorial(NyotaVal v) {
    int64_t acc = 1;
    int32_t n, i;
    if (v.type != TYPE_INT) {
        OutError("Silnia ! wymaga INTEGER");
        ValClear(&v);
        return v;
    }
    n = v.i;
    if (n < 0) {
        OutError("Silnia ! wymaga liczby nieujemnej");
        ValClear(&v);
        return v;
    }
    for (i = 2; i <= n; i++) {
        acc *= i;
        if (acc > INT32_MAX) {
            OutError("Silnia ! powoduje przepelnienie INTEGER");
            ValClear(&v);
            return v;
        }
    }
    ValFromInt(&v, (int32_t)acc);
    return v;
}

static NyotaVal ValBitOp(const NyotaVal *a, char op, const NyotaVal *b) {
    NyotaVal r;
    uint32_t av, bv;
    ValClear(&r);
    if (a->type != TYPE_INT || b->type != TYPE_INT) {
        OutError("Operatory bitowe wymagaja INTEGER");
        return r;
    }
    av = (uint32_t)a->i;
    bv = (uint32_t)b->i;
    if (op == 'L' || op == 'S') {
        if (b->i < 0 || b->i > 31) {
            OutError("SHL/SHR wymaga przesuniecia 0..31");
            return r;
        }
        if (op == 'L') ValFromInt(&r, (int32_t)(av << (uint32_t)b->i));
        else ValFromInt(&r, (int32_t)(av >> (uint32_t)b->i));
    } else if (op == '&') ValFromInt(&r, (int32_t)(av & bv));
    else if (op == 'X') ValFromInt(&r, (int32_t)(av ^ bv));
    else if (op == '|') ValFromInt(&r, (int32_t)(av | bv));
    else OutError("Nieznany operator bitowy");
    return r;
}

static NyotaVal ParseUnary(const char **pp) {
    const char *p = NTrim(*pp);
    if (p[0] == '-' && !NIsDigit(p[1])) {
        *pp = p + 1;
        return ValNeg(ParseUnary(pp));
    }
    *pp = p;
    return ParsePostfix(pp);
}

static NyotaVal ParsePostfix(const char **pp) {
    NyotaVal v = ParsePrimary(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        if (*p != '!') break;
        *pp = p + 1;
        v = ValFactorial(v);
    }
    return v;
}

static NyotaVal ParsePower(const char **pp) {
    NyotaVal left = ParseUnary(pp);
    const char *p = NTrim(*pp);
    if (p[0] == '^' && p[1] == '^') {
        *pp = p + 2;
        NyotaVal right = ParsePower(pp);
        return ValArith(&left, 'R', &right);
    }
    if (p[0] == '^') {
        *pp = p + 1;
        NyotaVal right = ParsePower(pp);
        return ValArith(&left, '^', &right);
    }
    return left;
}

static NyotaVal ParseMul(const char **pp) {
    NyotaVal left = ParsePower(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        char op;
        uint32_t n = 0;
        if (PeekWord(p, "MOD")) { op = 'M'; n = 3; }
        else if (p[0] == '/' && p[1] == '%') { op = 'M'; n = 2; }
        else if (p[0] == '*' || p[0] == '/' || p[0] == '%') { op = p[0]; n = 1; }
        else break;
        *pp = p + n;
        NyotaVal right = ParsePower(pp);
        left = ValArith(&left, op, &right);
    }
    return left;
}

static NyotaVal ParseAdd(const char **pp) {
    NyotaVal left = ParseMul(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        if (p[0] != '+' && p[0] != '-') break;
        char op = p[0];
        *pp = p + 1;
        if (left.type == TYPE_TIME) {
            const char *rhs = NTrim(*pp);
            int64_t shift = 0;
            uint32_t used = 0;
            if (ParseTimeShift(rhs, &shift, &used)) {
                ValFromTimeSeconds(&left, (int64_t)left.i + (op == '+' ? shift : -shift));
                *pp = rhs + used;
                continue;
            }
        }
        {
            NyotaVal right = ParseMul(pp);
            left = ValArith(&left, op, &right);
        }
    }
    return left;
}

static NyotaVal ParseShift(const char **pp) {
    NyotaVal left = ParseAdd(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        char op;
        uint32_t n;
        if (PeekWord(p, "SHL")) { op = 'L'; n = 3; }
        else if (PeekWord(p, "SHR")) { op = 'S'; n = 3; }
        else break;
        *pp = p + n;
        {
            NyotaVal right = ParseAdd(pp);
            left = ValBitOp(&left, op, &right);
        }
    }
    return left;
}

static NyotaVal ParseBand(const char **pp) {
    NyotaVal left = ParseShift(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        if (!PeekWord(p, "BAND")) break;
        *pp = p + 4;
        {
            NyotaVal right = ParseShift(pp);
            left = ValBitOp(&left, '&', &right);
        }
    }
    return left;
}

static NyotaVal ParseBxor(const char **pp) {
    NyotaVal left = ParseBand(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        if (!PeekWord(p, "BXOR")) break;
        *pp = p + 4;
        {
            NyotaVal right = ParseBand(pp);
            left = ValBitOp(&left, 'X', &right);
        }
    }
    return left;
}

static NyotaVal ParseBor(const char **pp) {
    NyotaVal left = ParseBxor(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        if (!PeekWord(p, "BOR")) break;
        *pp = p + 3;
        {
            NyotaVal right = ParseBxor(pp);
            left = ValBitOp(&left, '|', &right);
        }
    }
    return left;
}

static NyotaVal ParseCompare(const char **pp) {
    NyotaVal left = ParseBor(pp);
    const char *p = NTrim(*pp);
    if (PeekWord(p, "IN")) {
        *pp = p + 2;
        {
            NyotaVal right = ParseBor(pp);
            NyotaVal r;
            ValClear(&r);
            if (right.type == TYPE_MARK) {
                ValFromBool(&r, FindMarkRow(&right, &left) >= 0);
                return r;
            }
            if (right.type == TYPE_LIST || right.type == TYPE_TUPLE) {
                uint32_t i;
                int found = 0;
                for (i = 0; i < right.list_len; i++) {
                    if (ValEqual(&right.list_items[i], &left)) { found = 1; break; }
                }
                ValFromBool(&r, found);
                return r;
            }
            OutError("IN wymaga MARK, LIST albo TUPLE po prawej stronie");
            return r;
        }
    }
    p = NTrim(*pp);
    if (p[0] == '=' && p[1] == '=') {
        OutError("Operator == nie istnieje; uzyj =");
        *pp = p + 2;
        return left;
    }
    if (p[0] == '=' && NIsDigit(p[1])) {
        uint32_t k = 1;
        int n = 0;
        while (NIsDigit(p[k])) {
            n = n * 10 + (p[k] - '0');
            k++;
        }
        if (p[k] == '=') {
            NyotaVal right;
            int eq;
            *pp = p + k + 1;
            if (n < 0 || n > 3) {
                OutError("=N=: N musi byc w zakresie 0..3");
                ValClear(&left);
                return left;
            }
            right = ParseBor(pp);
            eq = ValEqN(&left, &right, n);
            if (eq < 0) {
                OutError("=N= wymaga INTEGER albo FLOAT tego samego typu");
                ValClear(&left);
                return left;
            }
            ValFromBool(&left, eq);
            return left;
        }
    }
    char op[4];
    uint32_t oplen = 0;
    if (!PeekCmpOp(p, op, &oplen)) return left;
    *pp = p + oplen;
    {
        NyotaVal right = ParseBor(pp);
        if (NStrEq(op, "><")) {
            if (left.type == TYPE_LIST && right.type == TYPE_LIST)
                return ListSymDiff(&left, &right);
            if (left.type == TYPE_MARK && right.type == TYPE_MARK)
                return MarkSymDiff(&left, &right);
            OutError("Operator >< wymaga dwoch LIST albo dwoch MARK; nierownosc zapisuj <>");
            ValClear(&left);
            return left;
        }
        return ValCompareOp(&left, op, &right);
    }
}

static NyotaVal ParseNot(const char **pp) {
    const char *p = NTrim(*pp);
    if (PeekWord(p, "NOT")) {
        *pp = p + 3;
        NyotaVal v = ParseNot(pp);
        ValFromBool(&v, !ValTruthy(&v));
        return v;
    }
    *pp = p;
    return ParseCompare(pp);
}

static NyotaVal ParseAnd(const char **pp) {
    NyotaVal left = ParseNot(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        if (!PeekWord(p, "AND")) break;
        *pp = p + 3;
        NyotaVal right = ParseNot(pp);
        ValFromBool(&left, ValTruthy(&left) && ValTruthy(&right));
    }
    return left;
}

static NyotaVal ParseOr(const char **pp) {
    NyotaVal left = ParseAnd(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        if (!PeekWord(p, "OR")) break;
        *pp = p + 2;
        NyotaVal right = ParseAnd(pp);
        ValFromBool(&left, ValTruthy(&left) || ValTruthy(&right));
    }
    return left;
}

static NyotaVal Eval(const char *expr_raw) {
    const char *p = NTrim(expr_raw);
    return ParseOr(&p);
}

// ============================================================
// EWALUACJA WARUNKÓW LOGICZNYCH
// ============================================================
static int32_t EvalBool(const char *expr) {
    NyotaVal v = Eval(expr);
    return ValTruthy(&v);
}

// ============================================================
// PRZYGOTOWANIE ŹRÓDŁA: Wczytaj linie
// ============================================================
static void ParseSourceToLines(void) {
    g_line_count = 0;
    uint32_t col = 0;
    for (uint32_t i = 0; i <= g_source_size; i++) {
        char c = (i < g_source_size) ? (char)g_source[i] : '\n';
        if (c == '\r') continue;
        if (c == '\n') {
            if (g_line_count < MAX_LINES) {
                g_lines[g_line_count][col] = '\0';
                NRTrim(g_lines[g_line_count]);
                g_line_count++;
            }
            col = 0;
        } else {
            if (col + 1 < MAX_LINE_LEN)
                g_lines[g_line_count][col++] = c;
        }
    }
}
static int ParseImportBufferLines(uint32_t size, uint32_t *out_count) {
    uint32_t li = 0, col = 0, i;
    for (i = 0; i <= size; i++) {
        char c = (i < size) ? g_import_buf[i] : '\n';
        if (c == '\r') continue;
        if (c == '\n') {
            if (li >= MAX_IMPORT_LINES) {
                OutError("IMPORT: plik ma za duzo linii");
                return 0;
            }
            g_import_lines[li][col] = '\0';
            NRTrim(g_import_lines[li]);
            li++;
            col = 0;
        } else {
            if (col + 1 >= MAX_LINE_LEN) {
                OutError("IMPORT: linia jest za dluga");
                return 0;
            }
            g_import_lines[li][col++] = c;
        }
    }
    *out_count = li;
    return 1;
}

static int ValidateImportedDeclarations(uint32_t count) {
    uint32_t i;
    int body_owner = 0;
    for (i = 0; i < count; i++) {
        const char *raw = g_import_lines[i];
        const char *line = NTrim(raw);
        uint32_t ind;
        if (!*line || *line == '#') continue;
        if (NLineHasTab(raw)) {
            OutError("IMPORT: tabulator jest zabroniony");
            return 0;
        }
        ind = NIndent(raw);
        if ((ind % NYOTA_INDENT) != 0) {
            OutError("IMPORT: wciecie musi byc wielokrotnoscia 4");
            return 0;
        }
        if (NStrEq(line, "BEGIN") || NStrEq(line, "END")) {
            OutError("IMPORT: plik importowany nie moze zawierac BEGIN/END");
            return 0;
        }
        if (NStartsWith(line, "IMPORT")) {
            OutError("IMPORT: import zagniezdzony jest zabroniony");
            return 0;
        }
        if (ind == 0) {
            if (NStartsWith(line, "CONST")) body_owner = 0;
            else if (NStartsWith(line, "RECORD") ||
                     NStartsWith(line, "FUNCTION") ||
                     NStartsWith(line, "PROCEDURE")) body_owner = 1;
            else {
                OutError("IMPORT: dozwolone sa tylko CONST, RECORD, FUNCTION i PROCEDURE");
                return 0;
            }
        } else if (!body_owner) {
            OutError("IMPORT: kod wykonywalny lub osierocone wciecie w pliku importowanym");
            return 0;
        }
    }
    return 1;
}

static int ExpandImports(void) {
    uint32_t i = 0;
    int saw_begin = 0;
    while (i < g_line_count) {
        const char *line = NTrim(g_lines[i]);
        if (NStrEq(line, "BEGIN")) saw_begin = 1;
        if (!NStartsWith(line, "IMPORT")) { i++; continue; }
        if (saw_begin || NIndent(g_lines[i]) != 0) {
            g_cur_line = i;
            OutError("IMPORT musi wystapic na poziomie 0 przed BEGIN");
            return 0;
        }
        {
            const char *p = NTrim(line + 6);
            char path[MAX_STR_LEN];
            uint32_t used = 0, got = 0, icount = 0, tail, new_count, k;
            int32_t rc;
            if (!ParseStringLit(p, path, sizeof(path), &used) || !path[0] ||
                *NTrim(p + used)) {
                g_cur_line = i;
                OutError("IMPORT wymaga jednej sciezki STRING");
                return 0;
            }
            rc = HostFileRead(path, g_import_buf, sizeof(g_import_buf), &got);
            if (rc != 0) {
                g_cur_line = i;
                OutError("IMPORT: nie mozna odczytac pliku");
                return 0;
            }
            if (!ParseImportBufferLines(got, &icount)) { g_cur_line = i; return 0; }
            if (!ValidateImportedDeclarations(icount)) { g_cur_line = i; return 0; }
            new_count = g_line_count - 1 + icount;
            if (new_count > MAX_LINES) {
                g_cur_line = i;
                OutError("IMPORT: laczny program przekracza limit linii");
                return 0;
            }
            tail = g_line_count - (i + 1);
            if (icount != 1) {
                int32_t src_index;
                for (src_index = (int32_t)tail - 1; src_index >= 0; src_index--) {
                    uint32_t from = i + 1 + (uint32_t)src_index;
                    uint32_t to = i + icount + (uint32_t)src_index;
                    NStrCopy(g_lines[to], g_lines[from], MAX_LINE_LEN);
                }
            }
            for (k = 0; k < icount; k++)
                NStrCopy(g_lines[i + k], g_import_lines[k], MAX_LINE_LEN);
            g_line_count = new_count;
            i += icount;
        }
    }
    return 1;
}


// ============================================================
// WYSZUKIWANIE PROCEDUR (pre-scan)
// ============================================================
static void ParseParamList(NyotaProc *p, const char *open_paren) {
    p->param_count = 0;
    const char *q = NTrim(open_paren);
    if (*q != '(') return;
    q++;
    char inner[256];
    uint32_t n = 0;
    int depth = 0;
    while (*q && n + 1 < sizeof(inner)) {
        if (*q == ')' && depth == 0) break;
        if (*q == '(') depth++;
        else if (*q == ')' && depth > 0) depth--;
        inner[n++] = *q++;
    }
    inner[n] = '\0';
    NRTrim(inner);
    if (!inner[0]) return;
    const char *s = inner;
    while (*s && p->param_count < 8) {
        char item[80];
        uint32_t k = 0;
        while (*s && *s != ',' && k + 1 < sizeof(item)) item[k++] = *s++;
        item[k] = '\0';
        const char *it = NTrim(item);
        uint8_t byref = 0;
        if (NStartsWith(it, "VAR")) {
            byref = 1;
            it = NTrim(it + 3);
        }
        char pname[64];
        ParseIdent(it, pname, sizeof(pname));
        if (pname[0]) {
            NStrCopy(p->params[p->param_count], pname, sizeof(p->params[0]));
            p->param_by_ref[p->param_count] = byref;
            p->param_count++;
        }
        if (*s == ',') s++;
        s = NTrim(s);
    }
}

static int ScanRecords(void) {
    uint32_t i;
    g_record_count = 0;
    for (i = 0; i < g_line_count; i++) {
        const char *line = NTrim(g_lines[i]);
        uint32_t def_indent, j;
        char name[64];
        uint32_t nlen;
        NyotaRecordDef *d;
        if (!NStartsWith(line, "RECORD")) continue;
        if (NIndent(g_lines[i]) != 0) {
            g_cur_line = i;
            OutError("RECORD musi byc deklarowany na poziomie 0 przed BEGIN");
            return 0;
        }
        if (g_record_count >= MAX_RECORDS) {
            g_cur_line = i;
            OutError("Za duzo definicji RECORD");
            return 0;
        }
        nlen = ParseIdent(NTrim(line + 6), name, sizeof(name));
        if (!name[0]) {
            g_cur_line = i;
            OutError("RECORD: brak nazwy");
            return 0;
        }
        {
            const char *tail = NTrim(NTrim(line + 6) + nlen);
            if (*tail != ':' || *NTrim(tail + 1)) {
                g_cur_line = i;
                OutError("RECORD: oczekiwano RECORD Nazwa:");
                return 0;
            }
        }
        if (FindRecordDef(name)) {
            g_cur_line = i;
            OutError("Duplikat RECORD");
            return 0;
        }
        d = &g_records[g_record_count++];
        NStrCopy(d->name, name, sizeof(d->name));
        d->field_count = 0;
        def_indent = NIndent(g_lines[i]);
        j = i + 1;
        while (j < g_line_count) {
            const char *raw = g_lines[j];
            const char *fl = NTrim(raw);
            uint32_t ind;
            char fname[64];
            uint32_t fn;
            const char *p;
            uint32_t k;
            if (!*fl || *fl == '#') { j++; continue; }
            ind = NIndent(raw);
            if (ind <= def_indent) break;
            if (ind != def_indent + NYOTA_INDENT) {
                g_cur_line = j;
                OutError("RECORD: pole musi miec dokladnie jedno wciecie +4");
                return 0;
            }
            if (d->field_count >= MAX_RECORD_FIELDS) {
                g_cur_line = j;
                OutError("RECORD: za duzo pol");
                return 0;
            }
            fn = ParseIdent(fl, fname, sizeof(fname));
            p = NTrim(fl + fn);
            if (!fname[0] || p[0] != ':' || p[1] != '=') {
                g_cur_line = j;
                OutError("RECORD: pole wymaga nazwa := wartosc");
                return 0;
            }
            for (k = 0; k < d->field_count; k++) {
                if (NStrEq(d->fields[k], fname)) {
                    g_cur_line = j;
                    OutError("RECORD: duplikat pola");
                    return 0;
                }
            }
            p = NTrim(p + 2);
            if (!*p) {
                g_cur_line = j;
                OutError("RECORD: brak wartosci domyslnej");
                return 0;
            }
            if (NStrLen(p) >= sizeof(d->defaults[0])) {
                g_cur_line = j;
                OutError("RECORD: wyrazenie domyslne pola jest za dlugie");
                return 0;
            }
            NStrCopy(d->fields[d->field_count], fname, sizeof(d->fields[0]));
            NStrCopy(d->defaults[d->field_count], p, sizeof(d->defaults[0]));
            d->field_count++;
            j++;
        }
        if (d->field_count == 0) {
            g_cur_line = i;
            OutError("RECORD wymaga co najmniej jednego pola");
            return 0;
        }
        i = j ? j - 1 : j;
    }
    return 1;
}

static void ScanProcedures(void) {
    g_proc_count = 0;
    for (uint32_t i = 0; i < g_line_count; i++) {
        const char *line = NTrim(g_lines[i]);
        int is_fn = NStartsWith(line, "FUNCTION");
        if (!is_fn && !NStartsWith(line, "PROCEDURE")) continue;
        if (g_proc_count >= MAX_PROCS) break;

        const char *np = is_fn ? NTrim(line + 8) : NTrim(line + 9);
        char name[64];
        uint32_t nlen = ParseIdent(np, name, sizeof(name));
        if (!name[0]) continue;
        if (FindRecordDef(name)) {
            g_cur_line = i;
            OutError("Nazwa FUNCTION/PROCEDURE koliduje z RECORD");
            continue;
        }

        uint32_t k;
        int dup = 0;
        for (k = 0; k < g_proc_count; k++) {
            if (NStrEq(g_procs[k].name, name)) { dup = 1; break; }
        }
        if (dup) {
            g_cur_line = i;
            OutError("Duplikat FUNCTION/PROCEDURE");
            continue;
        }

        NyotaProc *p = &g_procs[g_proc_count++];
        p->start_line = i;
        p->body_line = i + 1;
        p->is_function = is_fn ? 1 : 0;
        p->return_type = TYPE_NONE;
        p->return_type_known = 0;
        NStrCopy(p->name, name, sizeof(p->name));
        ParseParamList(p, NTrim(np + nlen));
    }
}
static int StaticExprType(const char *expr) {
    const char *p = NTrim(expr);
    uint32_t used = 0;
    NyotaVal v;
    int32_t y, m, d;
    if (!*p) return TYPE_NONE;
    if (*p == '"') return TYPE_STR;
    if (NStrEq(p, "TRUE") || NStrEq(p, "FALSE")) return TYPE_BOOL;
    if (*p == '[') return TYPE_LIST;
    if (*p == '{') return TYPE_MARK;
    if (*p == '(' && ParenLooksLikeTuple(p)) return TYPE_TUPLE;
    if (ParseDateLit(p, &y, &m, &d, &used)) return TYPE_DATE;
    if (NIsDigit(*p) || (*p == '-' && NIsDigit(p[1]))) {
        ValClear(&v);
        if (ParseNumber(p, &v, &used)) return v.type;
    }
    if (NStrEqN(p, "INT(", 4)) return TYPE_INT;
    if (NStrEqN(p, "FLT(", 4)) return TYPE_FLOAT;
    if (NStrEqN(p, "STR(", 4) || NStrEqN(p, "STRING(", 7)) return TYPE_STR;
    if (NStrEqN(p, "BOOL(", 5)) return TYPE_BOOL;
    if (NStrEqN(p, "LIST(", 5)) return TYPE_LIST;
    if (NStrEqN(p, "TUPLE(", 6)) return TYPE_TUPLE;
    if (NStrEqN(p, "TIME(", 5)) return TYPE_TIME;
    if (NStrEqN(p, "TODAY(", 6)) return TYPE_DATE;
    if (NIsAlpha(*p)) {
        char name[64];
        uint32_t n = ParseIdent(p, name, sizeof(name));
        const char *after = NTrim(p + n);
        if (*after == '(') {
            uint32_t i;
            for (i = 0; i < g_proc_count; i++) {
                if (g_procs[i].is_function && NStrEq(g_procs[i].name, name) &&
                    g_procs[i].return_type_known)
                    return g_procs[i].return_type;
            }
        }
    }
    return TYPE_NONE;
}

static int ValidateProcedureContracts(void) {
    uint32_t pass, pi;
    int ok = 1;

    /* Kilka przejść propaguje prosty typ z FUNCTION wywołującej inną FUNCTION.
     * Nie próbujemy zgadywać typu dowolnego wyrażenia z parametrami. */
    for (pass = 0; pass < 4; pass++) {
        for (pi = 0; pi < g_proc_count; pi++) {
            NyotaProc *p = &g_procs[pi];
            uint32_t def_indent = NIndent(g_lines[p->start_line]);
            uint32_t i = p->body_line;
            uint8_t found_type = p->return_type_known ? p->return_type : TYPE_NONE;
            int found_return = 0;

            while (i < g_line_count) {
                const char *raw = g_lines[i];
                const char *line = NTrim(raw);
                uint32_t ind;
                if (!*line || *line == '#') { i++; continue; }
                ind = NIndent(raw);
                if (ind <= def_indent) break;

                if (PeekWord(line, "RETURN")) {
                    const char *expr = NTrim(line + 6);
                    int t;
                    found_return = 1;
                    if (!p->is_function) {
                        if (pass == 0) {
                            g_cur_line = i;
                            OutError("RETURN w PROCEDURE jest zabroniony");
                            ok = 0;
                        }
                        i++;
                        continue;
                    }
                    if (!*expr) {
                        if (pass == 0) {
                            g_cur_line = i;
                            OutError("FUNCTION wymaga RETURN z wartoscia");
                            ok = 0;
                        }
                        i++;
                        continue;
                    }
                    t = StaticExprType(expr);
                    if (t != TYPE_NONE) {
                        if (found_type != TYPE_NONE && found_type != (uint8_t)t) {
                            if (pass == 0) {
                                g_cur_line = i;
                                OutError("FUNCTION ma sciezki RETURN zwracajace rozne typy");
                                ok = 0;
                            }
                        } else {
                            found_type = (uint8_t)t;
                        }
                    }
                }
                i++;
            }

            if (p->is_function && !found_return && pass == 0) {
                g_cur_line = p->start_line;
                OutError("FUNCTION bez RETURN");
                ok = 0;
            }
            if (found_type != TYPE_NONE) {
                p->return_type = found_type;
                p->return_type_known = 1;
            }
        }
    }
    return ok;
}


// ============================================================
// POMOCNICZE: PARSOWANIE ARGUMENTÓW PO PRZECINKU
// ============================================================
static int ParseArgs(const char *p, int32_t *args, int max_args) {
    int count = 0;
    while (*p && count < max_args) {
        char expr_buf[256]; uint32_t ei = 0; int depth = 0, in_str = 0;
        while (*p && (!(!in_str && depth == 0 && *p == ',') && ei + 1 < sizeof(expr_buf))) {
            if (*p == '"') in_str = !in_str;
            if (!in_str && (*p == '(' || *p == '[')) depth++;
            if (!in_str && (*p == ')' || *p == ']')) { if (depth > 0) depth--; }
            expr_buf[ei++] = *p++;
        }
        expr_buf[ei] = '\0';
        NyotaVal v = Eval(expr_buf);
        args[count++] = ValToInt(&v);
        if (*p == ',') p = NTrim(p + 1);
    }
    return count;
}


static NyotaWindow *FindWindow(const char *name) {
    uint32_t i;
    for (i = 0; i < g_window_count; i++)
        if (NStrEq(g_windows[i].name, name)) return &g_windows[i];
    return 0;
}

static int UiEvalPair(const char *expr, int32_t *a, int32_t *b, int positive, const char *label) {
    NyotaVal v = Eval(expr);
    if (v.type != TYPE_LIST || v.list_len != 2 ||
        v.list_items[0].type != TYPE_INT || v.list_items[1].type != TYPE_INT) {
        char err[160];
        NStrCopy(err, label, sizeof(err));
        NStrAppend(err, " wymaga LIST z dokladnie dwoma INTEGER", sizeof(err));
        OutError(err);
        return 0;
    }
    if (positive && (v.list_items[0].i <= 0 || v.list_items[1].i <= 0)) {
        char err[160];
        NStrCopy(err, label, sizeof(err));
        NStrAppend(err, " wymaga wartosci > 0", sizeof(err));
        OutError(err);
        return 0;
    }
    *a = v.list_items[0].i;
    *b = v.list_items[1].i;
    return 1;
}

static void UiBackgroundBlack(NyotaUiBackground *bg) {
    uint32_t i;
    if (!bg) return;
    bg->kind = NYOTA_UI_BG_COLOR;
    bg->image_mode = NYOTA_UI_IMG_CROP;
    bg->direction = NYOTA_UI_DIR_VERTICAL;
    bg->shape = NYOTA_UI_SHAPE_CIRCLE;
    bg->center_mode = NYOTA_UI_POS_CENTER;
    bg->spiral_direction = NYOTA_UI_SPIRAL_CW;
    bg->center_x = 0; bg->center_y = 0; bg->angle_deg = 0;
    bg->turns = 1;
    bg->color_count = 1;
    bg->image_path[0] = '\0';
    for (i = 0; i < NYOTA_UI_MAX_GRAD_COLORS; i++) {
        bg->colors[i].mode = NYOTA_COLOR_SOLID;
        bg->colors[i].r = bg->colors[i].g = bg->colors[i].b = 0;
        bg->colors[i].a = 255;
    }
}

static int UiParseColorList(const char *expr, uint32_t expected, NyotaUiBackground *bg) {
    NyotaVal list = Eval(expr);
    uint32_t i;
    if (expected < 2 || expected > NYOTA_UI_MAX_GRAD_COLORS) {
        OutError("GRAD: liczba kolorow musi byc 2..64");
        return 0;
    }
    if (list.type != TYPE_LIST || list.list_len != expected) {
        OutError("GRAD: liczba kolorow nie zgadza sie z dlugoscia LIST");
        return 0;
    }
    for (i = 0; i < expected; i++) {
        if (!ValToColorValue(&list.list_items[i], &bg->colors[i])) {
            OutError("GRAD: LIST moze zawierac tylko kolory Nyoty");
            return 0;
        }
        if (bg->colors[i].mode == NYOTA_COLOR_BACKDROP) {
            OutError("GRAD: BACKDROP nie moze byc punktem gradientu");
            return 0;
        }
    }
    bg->color_count = expected;
    return 1;
}

static int UiEvalInteger(const char *expr, int32_t *out, const char *label) {
    NyotaVal v = Eval(expr);
    if (v.type != TYPE_INT) {
        char err[128];
        NStrCopy(err, label, sizeof(err));
        NStrAppend(err, " wymaga INTEGER", sizeof(err));
        OutError(err);
        return 0;
    }
    *out = v.i;
    return 1;
}

static int UiParseCenter(const char *expr, NyotaUiBackground *bg) {
    const char *p = NTrim(expr);
    if (NStrEq(p, "CENTER")) {
        bg->center_mode = NYOTA_UI_POS_CENTER;
        bg->center_x = bg->center_y = 0;
        return 1;
    }
    bg->center_mode = NYOTA_UI_POS_XY;
    return UiEvalPair(p, &bg->center_x, &bg->center_y, 0, "GRAD: centrum");
}

static int UiParseDirection(const char *s, uint8_t *out) {
    s = NTrim(s);
    if (NStrEq(s, "VERTICAL")) *out = NYOTA_UI_DIR_VERTICAL;
    else if (NStrEq(s, "HORIZONTAL")) *out = NYOTA_UI_DIR_HORIZONTAL;
    else if (NStrEq(s, "DIAG_DOWN")) *out = NYOTA_UI_DIR_DIAG_DOWN;
    else if (NStrEq(s, "DIAG_UP")) *out = NYOTA_UI_DIR_DIAG_UP;
    else { OutError("GRAD LINEAR: kierunek VERTICAL/HORIZONTAL/DIAG_DOWN/DIAG_UP"); return 0; }
    return 1;
}

static int UiParseShape(const char *s, uint8_t *out) {
    s = NTrim(s);
    if (NStrEq(s, "CIRCLE")) *out = NYOTA_UI_SHAPE_CIRCLE;
    else if (NStrEq(s, "ELLIPSE")) *out = NYOTA_UI_SHAPE_ELLIPSE;
    else if (NStrEq(s, "SQUARE")) *out = NYOTA_UI_SHAPE_SQUARE;
    else if (NStrEq(s, "RECT")) *out = NYOTA_UI_SHAPE_RECT;
    else if (NStrEq(s, "DIAMOND")) *out = NYOTA_UI_SHAPE_DIAMOND;
    else if (NStrEq(s, "STAR")) *out = NYOTA_UI_SHAPE_STAR;
    else if (NStrEq(s, "EGG")) *out = NYOTA_UI_SHAPE_EGG;
    else { OutError("GRAD SHAPE: nieznany ksztalt"); return 0; }
    return 1;
}

static int UiParseImageMode(const char *s, uint8_t *out) {
    s = NTrim(s);
    if (NStrEq(s, "CROP")) *out = NYOTA_UI_IMG_CROP;
    else if (NStrEq(s, "FIT")) *out = NYOTA_UI_IMG_FIT;
    else if (NStrEq(s, "STRETCH")) *out = NYOTA_UI_IMG_STRETCH;
    else if (NStrEq(s, "NATIVE")) *out = NYOTA_UI_IMG_NATIVE;
    else if (NStrEq(s, "TILE")) *out = NYOTA_UI_IMG_TILE;
    else { OutError("IMG: tryb FIT/CROP/STRETCH/NATIVE/TILE"); return 0; }
    return 1;
}

static int UiParseBackground(const char *expr, NyotaUiBackground *bg) {
    char work[MAX_LINE_LEN];
    const char *p;
    uint32_t len;
    UiBackgroundBlack(bg);
    NStrCopy(work, NTrim(expr), sizeof(work));
    NRTrim(work);
    p = work;
    len = NStrLen(work);

    if (NStrEqN(p, "IMG(", 4)) {
        char args[2][MAX_STR_LEN];
        int n;
        NyotaVal pathv;
        if (len < 5 || work[len - 1] != ')') { OutError("IMG: brak zamykajacego )"); return 0; }
        n = SplitFunctionArgs(p + 4, args, 2);
        if (n != 1 && n != 2) { OutError("IMG(path [, tryb])"); return 0; }
        pathv = Eval(args[0]);
        if (pathv.type != TYPE_STR || !pathv.s[0]) { OutError("IMG: path wymaga niepustego STRING"); return 0; }
        bg->kind = NYOTA_UI_BG_IMAGE;
        NStrCopy(bg->image_path, pathv.s, sizeof(bg->image_path));
        bg->image_mode = NYOTA_UI_IMG_CROP;
        if (n == 2 && !UiParseImageMode(args[1], &bg->image_mode)) return 0;
        return 1;
    }

    if (NStrEqN(p, "GRAD(", 5)) {
        char args[7][MAX_STR_LEN];
        int n;
        int32_t count, angle, turns;
        if (len < 6 || work[len - 1] != ')') { OutError("GRAD: brak zamykajacego )"); return 0; }
        n = SplitFunctionArgs(p + 5, args, 7);
        if (n <= 0) { OutError("GRAD: brak argumentow"); return 0; }

        if (NStrEq(NTrim(args[0]), "LINEAR")) {
            if (n != 4) { OutError("GRAD(LINEAR, kierunek, liczba, kolory)"); return 0; }
            bg->kind = NYOTA_UI_BG_LINEAR;
            if (!UiParseDirection(args[1], &bg->direction)) return 0;
            if (!UiEvalInteger(args[2], &count, "GRAD: liczba kolorow")) return 0;
            return UiParseColorList(args[3], (uint32_t)count, bg);
        }

        if (NStrEq(NTrim(args[0]), "SHAPE")) {
            if (n != 6) { OutError("GRAD(SHAPE, figura, centrum, kat, liczba, kolory)"); return 0; }
            bg->kind = NYOTA_UI_BG_SHAPE;
            if (!UiParseShape(args[1], &bg->shape)) return 0;
            if (!UiParseCenter(args[2], bg)) return 0;
            if (!UiEvalInteger(args[3], &angle, "GRAD: kat")) return 0;
            if (!UiEvalInteger(args[4], &count, "GRAD: liczba kolorow")) return 0;
            bg->angle_deg = angle;
            return UiParseColorList(args[5], (uint32_t)count, bg);
        }

        if (NStrEq(NTrim(args[0]), "SPIRAL")) {
            if (n != 7) { OutError("GRAD(SPIRAL, centrum, kat, obroty, CW/CCW, liczba, kolory)"); return 0; }
            bg->kind = NYOTA_UI_BG_SPIRAL;
            if (!UiParseCenter(args[1], bg)) return 0;
            if (!UiEvalInteger(args[2], &angle, "GRAD: kat")) return 0;
            if (!UiEvalInteger(args[3], &turns, "GRAD SPIRAL: obroty")) return 0;
            if (turns <= 0) { OutError("GRAD SPIRAL: obroty musza byc > 0"); return 0; }
            if (NStrEq(NTrim(args[4]), "CW")) bg->spiral_direction = NYOTA_UI_SPIRAL_CW;
            else if (NStrEq(NTrim(args[4]), "CCW")) bg->spiral_direction = NYOTA_UI_SPIRAL_CCW;
            else { OutError("GRAD SPIRAL: kierunek CW albo CCW"); return 0; }
            if (!UiEvalInteger(args[5], &count, "GRAD: liczba kolorow")) return 0;
            bg->angle_deg = angle;
            bg->turns = (uint32_t)turns;
            return UiParseColorList(args[6], (uint32_t)count, bg);
        }

        OutError("GRAD: pierwszy argument musi byc LINEAR, SHAPE albo SPIRAL");
        return 0;
    }

    {
        NyotaVal cv = Eval(p);
        NyotaColor color;
        if (!ValToColorValue(&cv, &color)) {
            OutError("BG wymaga koloru, IMG(...) albo GRAD(...)");
            return 0;
        }
        bg->kind = NYOTA_UI_BG_COLOR;
        bg->color_count = 1;
        bg->colors[0] = color;
        return 1;
    }
}

static int UiApplyWinConfig(uint32_t ln, const char *raw, NyotaWindow *w) {
    uint32_t my_indent = NIndent(raw);
    uint32_t body_start = ln + 1;
    uint32_t body_end = SkipBlock(body_start, my_indent);
    uint32_t i;
    g_cur_line = body_end;
    for (i = body_start; i < body_end; i++) {
        const char *cl = NTrim(g_lines[i]);
        char prop[64];
        uint32_t pn;
        const char *rhs;
        if (!*cl || *cl == '#') continue;
        if (NIndent(g_lines[i]) != my_indent + NYOTA_INDENT) {
            g_cur_line = i; OutError("WIN CONFIG: wymagane dokladnie 4 spacje"); return 0;
        }
        pn = ParseIdent(cl, prop, sizeof(prop));
        rhs = NTrim(cl + pn);
        if (!prop[0] || *rhs != '=') {
            g_cur_line = i; OutError("WIN CONFIG: wymagane WLASCIWOSC = wartosc"); return 0;
        }
        rhs = NTrim(rhs + 1);
        g_cur_line = i;
        if (NStrEq(prop, "TITLE")) {
            NyotaVal v = Eval(rhs);
            if (v.type != TYPE_STR) { OutError("WIN CONFIG TITLE wymaga STRING"); return 0; }
            NStrCopy(w->title, v.s, sizeof(w->title));
            if (HostWinSetTitle(w->host_handle, w->title) != 0) {
                OutError("WIN CONFIG TITLE: host odrzucil ustawienie"); return 0;
            }
        } else if (NStrEq(prop, "ICO")) {
            NyotaVal v = Eval(rhs);
            if (v.type != TYPE_STR || !v.s[0]) { OutError("WIN CONFIG ICO wymaga niepustego STRING"); return 0; }
            NStrCopy(w->icon, v.s, sizeof(w->icon));
            if (HostWinSetIcon(w->host_handle, w->icon) != 0) {
                OutError("WIN CONFIG ICO: nie mozna ustawic ikony"); return 0;
            }
        } else if (NStrEq(prop, "BG")) {
            NyotaUiBackground bg;
            if (!UiParseBackground(rhs, &bg)) return 0;
            w->background = bg;
            if (HostWinSetBackground(w->host_handle, &w->background) != 0) {
                OutError("WIN CONFIG BG: host odrzucil tlo"); return 0;
            }
        } else if (NStrEq(prop, "SHADOW")) {
            const char *v = NTrim(rhs);
            if (NStrEq(v, "OFF")) w->shadow = NYOTA_UI_SHADOW_OFF;
            else if (NStrEq(v, "R")) w->shadow = NYOTA_UI_SHADOW_R;
            else if (NStrEq(v, "L")) w->shadow = NYOTA_UI_SHADOW_L;
            else if (NStrEq(v, "U")) w->shadow = NYOTA_UI_SHADOW_U;
            else if (NStrEq(v, "D")) w->shadow = NYOTA_UI_SHADOW_D;
            else if (NStrEq(v, "RU")) w->shadow = NYOTA_UI_SHADOW_RU;
            else if (NStrEq(v, "RD")) w->shadow = NYOTA_UI_SHADOW_RD;
            else if (NStrEq(v, "LU")) w->shadow = NYOTA_UI_SHADOW_LU;
            else if (NStrEq(v, "LD")) w->shadow = NYOTA_UI_SHADOW_LD;
            else { OutError("WIN CONFIG SHADOW wymaga OFF/R/L/U/D/RU/RD/LU/LD"); return 0; }
        } else if (NStrEq(prop, "CSHADOW")) {
            NyotaVal v = Eval(rhs);
            if (!ValToColorValue(&v, &w->shadow_color) || w->shadow_color.mode == NYOTA_COLOR_BACKDROP) {
                OutError("WIN CONFIG CSHADOW wymaga koloru Nyoty"); return 0;
            }
        } else if (NStrEq(prop, "SDEPTH")) {
            NyotaVal v = Eval(rhs);
            if (v.type != TYPE_INT || v.i < 1 || v.i > 64) {
                OutError("WIN CONFIG SDEPTH wymaga INTEGER 1..64"); return 0;
            }
            w->shadow_depth = (uint32_t)v.i;
        } else {
            OutError("WIN CONFIG: nieznana wlasciwosc (TITLE/BG/ICO/SHADOW/CSHADOW/SDEPTH)"); return 0;
        }
    }
    /* SHADOW w WIN.CONFIG jest polityka cienia kontrolek tego okna. */
    for (i = 0; i < g_ui_control_count; i++) {
        if (g_ui_controls[i].window_handle == w->host_handle) {
            g_ui_controls[i].spec.shadow = w->shadow;
            g_ui_controls[i].spec.shadow_color = w->shadow_color;
            g_ui_controls[i].spec.shadow_depth = w->shadow_depth;
            if (g_ui_controls[i].host_handle > 0)
                HostUiControlUpdate(g_ui_controls[i].host_handle, &g_ui_controls[i].spec);
        }
    }
    g_cur_line = body_end;
    return 1;
}

static int UiExecWin(uint32_t ln, const char *raw, const char *line) {
    const char *p = NTrim(line + 3);
    char name[64];
    uint32_t nn = ParseIdent(p, name, sizeof(name));
    NyotaWindow *existing;
    p = NTrim(p + nn);
    if (!name[0]) { OutError("WIN: brak nazwy okna"); return 1; }

    if (NStrEq(p, ".CONFIG:")) {
        existing = FindWindow(name);
        if (!existing) {
            g_cur_line = SkipBlock(ln + 1, NIndent(raw));
            OutError("WIN CONFIG: nieznane okno");
            return 1;
        }
        UiApplyWinConfig(ln, raw, existing);
        return 1;
    }

    if (*p != ',') { OutError("WIN: po nazwie wymagany przecinek"); return 1; }
    if (FindWindow(name)) { OutError("WIN: nazwa okna juz istnieje"); return 1; }
    if (g_window_count >= MAX_WINDOWS) { OutError("WIN: przekroczono limit 32 okien"); return 1; }
    if (!g_host || !g_host->ui_win_create || !g_host->ui_win_set_background) {
        OutError("WIN: host nie obsluguje NyotaUI WIN");
        return 1;
    }

    {
        char args[4][MAX_STR_LEN];
        int n = SplitFunctionArgs(NTrim(p + 1), args, 4);
        char parent_name[64];
        uint32_t pn;
        NyotaWindow *parent = 0;
        int32_t parent_handle = 0;
        int32_t wi, hi, x = 0, y = 0;
        uint8_t pos_mode = NYOTA_UI_POS_SYSTEM;
        uint8_t resizable = 1;
        int32_t handle;
        NyotaWindow temp;

        if (n < 2 || n > 4) {
            OutError("WIN nazwa, rodzic, [w,h] [, CENTER/[x,y]] [, TRUE/FALSE]");
            return 1;
        }

        pn = ParseIdent(NTrim(args[0]), parent_name, sizeof(parent_name));
        if (!parent_name[0] || *NTrim(NTrim(args[0]) + pn)) {
            OutError("WIN: rodzic musi byc ROOT albo nazwa WIN");
            return 1;
        }
        if (!NStrEq(parent_name, "ROOT")) {
            parent = FindWindow(parent_name);
            if (!parent) { OutError("WIN: nieznany rodzic"); return 1; }
            parent_handle = parent->host_handle;
        }

        if (!UiEvalPair(args[1], &wi, &hi, 1, "WIN: rozmiar")) return 1;

        if (n == 3) {
            NyotaVal third;
            if (NStrEq(NTrim(args[2]), "CENTER")) {
                pos_mode = NYOTA_UI_POS_CENTER;
            } else {
                third = Eval(args[2]);
                if (third.type == TYPE_BOOL) {
                    resizable = (uint8_t)(third.i != 0);
                } else if (third.type == TYPE_LIST && third.list_len == 2 &&
                           third.list_items[0].type == TYPE_INT && third.list_items[1].type == TYPE_INT) {
                    x = third.list_items[0].i; y = third.list_items[1].i;
                    pos_mode = NYOTA_UI_POS_XY;
                } else {
                    OutError("WIN: trzeci argument wymaga CENTER, [x,y] albo BOOLEAN");
                    return 1;
                }
            }
        } else if (n == 4) {
            NyotaVal rv;
            if (NStrEq(NTrim(args[2]), "CENTER")) pos_mode = NYOTA_UI_POS_CENTER;
            else {
                if (!UiEvalPair(args[2], &x, &y, 0, "WIN: pozycja")) return 1;
                pos_mode = NYOTA_UI_POS_XY;
            }
            rv = Eval(args[3]);
            if (rv.type != TYPE_BOOL) { OutError("WIN: resize wymaga BOOLEAN"); return 1; }
            resizable = (uint8_t)(rv.i != 0);
        }

        handle = HostWinCreate(name, parent_handle, (uint32_t)wi, (uint32_t)hi,
                               x, y, pos_mode, resizable);
        if (handle <= 0) { OutError("WIN: host nie utworzyl okna"); return 1; }

        temp.host_handle = handle;
        temp.w = (uint32_t)wi; temp.h = (uint32_t)hi;
        temp.x = x; temp.y = y; temp.position_mode = pos_mode; temp.resizable = resizable;
        NStrCopy(temp.name, name, sizeof(temp.name));
        NStrCopy(temp.parent_name, parent_name, sizeof(temp.parent_name));
        NStrCopy(temp.title, name, sizeof(temp.title));
        temp.icon[0] = '\0';
        UiBackgroundBlack(&temp.background);
        temp.shadow = NYOTA_UI_SHADOW_OFF;
        temp.shadow_color.mode = NYOTA_COLOR_SOLID;
        temp.shadow_color.r = 0; temp.shadow_color.g = 0; temp.shadow_color.b = 0; temp.shadow_color.a = 160;
        temp.shadow_depth = 2;

        if (HostWinSetBackground(handle, &temp.background) != 0) {
            HostWinDestroy(handle);
            OutError("WIN: host nie ustawil domyslnego tla");
            return 1;
        }
        if (g_host->ui_win_set_title && HostWinSetTitle(handle, temp.title) != 0) {
            HostWinDestroy(handle);
            OutError("WIN: host nie ustawil tytulu");
            return 1;
        }
        g_windows[g_window_count++] = temp;
    }
    return 1;
}

// ============================================================
// NYOTAUI — kontrolki przenosne
// ============================================================
#define NYOTA_UI_CHECK_BASE 20u

static NyotaUiControl *FindUiControl(const char *name) {
    uint32_t i;
    for (i = 0; i < g_ui_control_count; i++)
        if (NStrEq(g_ui_controls[i].name, name)) return &g_ui_controls[i];
    return 0;
}

static void UiColorSolid(NyotaColor *c, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    c->mode = NYOTA_COLOR_SOLID; c->r = r; c->g = g; c->b = b; c->a = a;
}

static void UiBackgroundTransparent(NyotaUiBackground *bg) {
    UiBackgroundBlack(bg);
    bg->colors[0].mode = NYOTA_COLOR_TRANSPARENT;
    bg->colors[0].a = 0;
}

static void UiControlDefaults(NyotaUiControlSpec *s, uint8_t kind) {
    memset(s, 0, sizeof(*s));
    s->kind = kind;
    s->position_mode = NYOTA_UI_POS_XY;

    /* NyotaUI default theme: neutral dark desktop look. */
    NStrCopy(s->font, "SYSTEM", sizeof(s->font));
    s->font_size = 14;
    UiColorSolid(&s->text_color, 232, 236, 242, 255);
    s->halign = NYOTA_UI_ALIGN_LEFT;
    s->valign = NYOTA_UI_VALIGN_MIDDLE;
    s->enabled = 1;
    s->pad_x = 8;
    s->pad_y = 4;

    UiBackgroundBlack(&s->background);
    UiColorSolid(&s->background.colors[0], 36, 41, 50, 255);
    UiBackgroundBlack(&s->background_over);
    UiColorSolid(&s->background_over.colors[0], 48, 56, 69, 255);

    s->border = 0;
    UiColorSolid(&s->border_color, 76, 86, 102, 255);
    s->border_width = 1;
    UiColorSolid(&s->border_color_over, 108, 132, 170, 255);
    s->border_width_over = 1;

    s->clip = 1;
    s->shape = NYOTA_UI_DAREA_RECT;
    s->accept = NYOTA_UI_ACCEPT_ALL;
    s->multi = 1;
    s->radius = 0;
    s->layout = NYOTA_UI_LAYOUT_FREE;
    s->gap = 8;
    s->layout_pad_x = 8;
    s->layout_pad_y = 8;
    s->readonly = 0;
    UiColorSolid(&s->caret_color, 235, 240, 248, 255);
    s->logical_size = 1;
    s->checked = 0;

    UiColorSolid(&s->check_color, 86, 156, 232, 255);
    NStrCopy(s->check_symbol, "X", sizeof(s->check_symbol));

    s->selected = 0;
    s->max_visible = 8;
    s->hover_enabled = 1;

    UiBackgroundBlack(&s->drop_background);
    UiColorSolid(&s->drop_background.colors[0], 31, 36, 44, 255);
    UiColorSolid(&s->drop_text_color, 228, 232, 238, 255);

    UiBackgroundBlack(&s->selected_background);
    UiColorSolid(&s->selected_background.colors[0], 49, 91, 145, 255);
    UiColorSolid(&s->selected_text_color, 255, 255, 255, 255);

    UiBackgroundBlack(&s->hover_background);
    UiColorSolid(&s->hover_background.colors[0], 49, 59, 73, 255);
    UiColorSolid(&s->hover_text_color, 255, 255, 255, 255);
    UiColorSolid(&s->arrow_color, 205, 212, 222, 255);

    s->orientation = NYOTA_UI_SEP_HORIZONTAL;
    s->sep_effect = NYOTA_UI_SEP_NORMAL;
    UiColorSolid(&s->sep_color, 74, 82, 94, 255);
    s->sep_thickness = 1;

    s->range_min = 0;
    s->range_max = 100;
    s->range_value = 0;
    s->range_page = 10;
    s->range_step = 1;
    s->thumb_shape = NYOTA_UI_SBAR_THUMB_ROUND;
    UiColorSolid(&s->thumb_color, 95, 156, 232, 255);
    UiColorSolid(&s->thumb_hover_color, 126, 183, 250, 255);

    s->progress_shape = NYOTA_UI_PBAR_SHAPE_BAR;
    s->progress_segments = 20;
    s->progress_gap = 4;
    UiColorSolid(&s->progress_fill_color, 38, 196, 128, 255);
    UiColorSolid(&s->progress_empty_color, 69, 76, 88, 255);

    s->slider_thumb_size = 18;

    s->max_length = NYOTA_UI_TEXT_MAX - 1u;
    s->password = 0;

    s->signed_min = 0;
    s->signed_max = 100;
    s->signed_value = 0;
    s->signed_step = 1;

    s->row_height = 28;
    s->tree_indent = 20;
    s->show_lines = 1;
    s->splitter_thickness = 6;

    s->scale_shape = NYOTA_UI_SCALE_LINE;
    s->scale_wrap = 0;
    s->scale_interactive = 1;
    s->scale_radius = 90;
    s->start_angle = 210;
    s->end_angle = 510;
    s->track_style = NYOTA_UI_TRACK_SOLID;
    s->track_width = 6;
    s->track_gap = 4;
    UiBackgroundBlack(&s->track_fill);
    UiColorSolid(&s->track_fill.colors[0], 82, 92, 108, 255);
    UiBackgroundTransparent(&s->track_glow);
    s->track_blur = 0;
    s->scale_thumb_shape = NYOTA_UI_SCALE_THUMB_CIRCLE;
    s->thumb_rotate = 1;
    s->thumb_align = NYOTA_UI_THUMB_ALIGN_RADIAL;
    s->scale_thumb_size = 18;
    s->scale_thumb_width = 3;
    UiBackgroundBlack(&s->scale_thumb_fill);
    UiColorSolid(&s->scale_thumb_fill.colors[0], 110, 174, 244, 255);
    s->scale_thumb_border = 0;
    UiColorSolid(&s->scale_thumb_border_color, 235, 240, 248, 255);
    s->scale_thumb_border_width = 1;
    UiBackgroundTransparent(&s->scale_thumb_glow);
    s->scale_thumb_blur = 0;

    s->major_step = 10;
    s->minor_step = 5;
    s->tick_style = NYOTA_UI_TICK_LINE;
    UiColorSolid(&s->tick_color, 226, 232, 240, 255);
    UiColorSolid(&s->minor_tick_color, 120, 130, 145, 255);
    s->tick_len = 12;
    s->minor_tick_len = 6;
    s->tick_width = 2;
    s->tick_blur = 0;
    s->show_labels = 1;
    s->label_step = 10;
    s->label_offset = 18;

    s->clock_shape = NYOTA_UI_CLOCK_ARC;
    s->clock_radius = 90;
    s->clock_needles = 1;
    s->clock_value_count = 1;
    s->clock_values[0] = 0;
    s->clock_color_count = 1;
    s->clock_shape_count = 1;
    s->clock_width_count = 1;
    s->clock_len_count = 1;
    s->clock_blur_count = 1;
    s->clock_min_count = 1;
    s->clock_max_count = 1;
    s->clock_needle_shape[0] = NYOTA_UI_NEEDLE_LINE;
    UiColorSolid(&s->clock_needle_color[0], 235, 74, 86, 255);
    s->clock_needle_width[0] = 3;
    s->clock_needle_len[0] = 82;
    s->clock_needle_blur[0] = 0;
    s->clock_needle_min[0] = 0;
    s->clock_needle_max[0] = 100;
    s->dial_blur = 0;
    s->center_dot = 1;
    UiColorSolid(&s->center_color, 235, 240, 248, 255);
    s->center_size = 10;
    s->center_blur = 0;
    s->show_value = 1;
    s->unit[0] = '\0';
    s->zone_count = 0;

    s->eq_bars = 8;
    s->eq_bar_width = 18;
    s->eq_gap = 6;
    s->eq_segment_gap = 2;
    s->eq_min_height = 0;
    s->eq_max_height = 0;
    s->eq_blur = 0;
    s->eq_build = NYOTA_UI_EQ_SOLID;
    s->eq_direction = NYOTA_UI_EQ_UP;
    s->eq_label_pos = NYOTA_UI_EQ_LABEL_BOTTOM;
    s->eq_show_labels = 0;
    s->eq_show_values = 0;
    s->eq_value_count = 0;
    s->eq_label_count = 0;
    s->eq_color_count = 0;
    UiBackgroundBlack(&s->eq_bar_background);
    UiColorSolid(&s->eq_bar_background.colors[0], 45, 51, 61, 255);
    UiBackgroundBlack(&s->eq_bar_fill);
    UiColorSolid(&s->eq_bar_fill.colors[0], 38, 196, 128, 255);
    UiBackgroundTransparent(&s->eq_glow);

    UiBackgroundBlack(&s->tab_background);
    UiColorSolid(&s->tab_background.colors[0], 43, 49, 59, 255);
    NStrCopy(s->tab_font, "SYSTEM", sizeof(s->tab_font));
    s->tab_font_size = 14;
    UiColorSolid(&s->tab_text_color, 225, 230, 237, 255);
    s->tab_border = 1;
    UiColorSolid(&s->tab_border_color, 76, 86, 102, 255);
    s->tab_border_width = 1;
    s->tab_radius = 6;
    s->tab_pad_x = 12;
    s->tab_pad_y = 6;

    s->shadow = NYOTA_UI_SHADOW_OFF;
    UiColorSolid(&s->shadow_color, 0, 0, 0, 120);
    s->shadow_depth = 2;

    if (kind == NYOTA_UI_CTRL_BUTTON) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 52, 63, 78, 255);
        s->halign = NYOTA_UI_ALIGN_CENTER;
        s->border = 1;
        s->radius = 6;
        s->bold = 0;
    } else if (kind == NYOTA_UI_CTRL_LABEL) {
        UiBackgroundTransparent(&s->background);
        s->border = 0;
    } else if (kind == NYOTA_UI_CTRL_PANEL) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 31, 35, 43, 255);
        s->border = 1;
        s->radius = 8;
    } else if (kind == NYOTA_UI_CTRL_TAB) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 31, 35, 43, 255);
        s->border = 1;
        s->radius = 8;
    } else if (kind == NYOTA_UI_CTRL_DAREA) {
        UiColorSolid(&s->text_color, 184, 193, 205, 255);
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 27, 31, 38, 255);
        UiBackgroundBlack(&s->background_over);
        UiColorSolid(&s->background_over.colors[0], 37, 47, 61, 255);
        s->border = 1;
        s->radius = 0;
    } else if (kind == NYOTA_UI_CTRL_CBOX || kind == NYOTA_UI_CTRL_RADIO) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 238, 241, 245, 255);
        s->border = 1;
        s->radius = (kind == NYOTA_UI_CTRL_CBOX) ? 4 : 0;
    } else if (kind == NYOTA_UI_CTRL_COMBO) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 42, 48, 58, 255);
        s->border = 1;
        s->radius = 6;
    } else if (kind == NYOTA_UI_CTRL_TAREA) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 26, 30, 37, 255);
        s->border = 1;
        s->radius = 6;
        s->wrap = 1;
        s->valign = NYOTA_UI_VALIGN_TOP;
    } else if (kind == NYOTA_UI_CTRL_SBAR) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 28, 33, 41, 255);
        s->border = 1;
        s->radius = 6;
    } else if (kind == NYOTA_UI_CTRL_PBAR) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 25, 29, 36, 255);
        s->border = 1;
        s->radius = 6;
    } else if (kind == NYOTA_UI_CTRL_SLIDER) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 28, 33, 41, 255);
        s->border = 1;
        s->radius = 6;
    } else if (kind == NYOTA_UI_CTRL_EQBOX) {
        s->orientation = NYOTA_UI_SEP_VERTICAL;
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 18, 22, 29, 255);
        s->border = 1;
        s->radius = 8;
    } else if (kind == NYOTA_UI_CTRL_STATBAR || kind == NYOTA_UI_CTRL_TOOLBAR) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], kind==NYOTA_UI_CTRL_STATBAR?31:36, kind==NYOTA_UI_CTRL_STATBAR?35:41, kind==NYOTA_UI_CTRL_STATBAR?43:50, 255);
        s->border = 0;
        s->radius = 0;
        s->layout = NYOTA_UI_LAYOUT_ROW;
        s->gap = 6;
        s->layout_pad_x = 8;
        s->layout_pad_y = 4;
    } else if (kind == NYOTA_UI_CTRL_TBOX) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 26, 30, 37, 255);
        s->border = 1;
        s->radius = 6;
        s->wrap = 0;
        s->valign = NYOTA_UI_VALIGN_MIDDLE;
    } else if (kind == NYOTA_UI_CTRL_SPINBOX) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 30, 35, 43, 255);
        s->border = 1;
        s->radius = 6;
        s->halign = NYOTA_UI_ALIGN_RIGHT;
    } else if (kind == NYOTA_UI_CTRL_LISTVIEW || kind == NYOTA_UI_CTRL_TREEVIEW) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 25, 29, 36, 255);
        s->border = 1;
        s->radius = 6;
        s->multi = 0;
    } else if (kind == NYOTA_UI_CTRL_SPLITTER) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 62, 72, 86, 255);
        s->border = 0;
        s->radius = 2;
        s->range_min = 0;
        s->range_max = 1000;
        s->range_value = 500;
        s->range_step = 1;
    } else if (kind == NYOTA_UI_CTRL_SCALE) {
        UiBackgroundTransparent(&s->background);
        s->border = 0;
        s->signed_min = 0;
        s->signed_max = 100;
        s->signed_value = 0;
        s->signed_step = 1;
    } else if (kind == NYOTA_UI_CTRL_CLOCK) {
        UiBackgroundBlack(&s->background);
        UiColorSolid(&s->background.colors[0], 20, 24, 31, 255);
        s->border = 1;
        s->radius = 0;
        s->halign = NYOTA_UI_ALIGN_CENTER;
    }
}

static int UiParseColorProperty(const char *rhs, NyotaColor *out, const char *label) {
    NyotaVal v=Eval(rhs);
    if(!ValToColorValue(&v,out)||out->mode==NYOTA_COLOR_BACKDROP){char err[160];NStrCopy(err,label,sizeof(err));NStrAppend(err," wymaga koloru Nyoty",sizeof(err));OutError(err);return 0;} return 1;
}
static int UiParseBoolProperty(const char *rhs,uint8_t *out,const char *label){NyotaVal v=Eval(rhs);if(v.type!=TYPE_BOOL){char e[160];NStrCopy(e,label,sizeof(e));NStrAppend(e," wymaga BOOLEAN",sizeof(e));OutError(e);return 0;}*out=(uint8_t)(v.i!=0);return 1;}
static int UiParseUIntProperty(const char *rhs,uint32_t *out,uint32_t minv,uint32_t maxv,const char *label){NyotaVal v=Eval(rhs);if(v.type!=TYPE_INT||v.i<(int32_t)minv||v.i>(int32_t)maxv){char e[180];NStrCopy(e,label,sizeof(e));NStrAppend(e," ma nieprawidlowa wartosc",sizeof(e));OutError(e);return 0;}*out=(uint32_t)v.i;return 1;}
static int UiParseIntProperty(const char *rhs,int32_t *out,int32_t minv,int32_t maxv,const char *label){NyotaVal v=Eval(rhs);if(v.type!=TYPE_INT||v.i<minv||v.i>maxv){char e[180];NStrCopy(e,label,sizeof(e));NStrAppend(e," ma nieprawidlowa wartosc",sizeof(e));OutError(e);return 0;}*out=v.i;return 1;}
static int UiParseStringProperty(const char *rhs,char *out,uint32_t cap,const char *label){NyotaVal v=Eval(rhs);if(v.type!=TYPE_STR){char e[180];NStrCopy(e,label,sizeof(e));NStrAppend(e," wymaga STRING",sizeof(e));OutError(e);return 0;}NStrCopy(out,v.s,cap);return 1;}

static int UiListToItems(const NyotaVal *v,char *out,uint32_t cap){
    uint32_t i,used=0;
    if(!v||v->type!=TYPE_LIST){OutError("ITEMS wymaga LIST");return 0;}
    out[0]='\0';
    for(i=0;i<v->list_len;i++){
        uint32_t n;
        if(v->list_items[i].type!=TYPE_STR){OutError("ITEMS wymaga LIST elementow STRING");return 0;}
        n=NStrLen(v->list_items[i].s);
        if(used+n+2>cap){OutError("ITEMS: lista tekstow jest za dluga");return 0;}
        memcpy(out+used,v->list_items[i].s,n);used+=n;out[used++]='\n';out[used]='\0';
    }
    return 1;
}
static uint32_t UiItemsCount(const char *s){uint32_t n=0,i=0;if(!s)return 0;while(s[i]){if(s[i++]=='\n')n++;}return n;}
static int UiItemText(const char *items,uint32_t wanted,char *out,uint32_t cap){
    uint32_t cur=0,start=0,i=0;if(!out||!cap)return 0;out[0]='\0';
    while(items&&items[i]){
        if(items[i]=='\n'){
            if(cur==wanted){uint32_t n=i-start;if(n>=cap)n=cap-1;memcpy(out,items+start,n);out[n]='\0';return 1;}
            cur++;start=i+1;
        }
        i++;
    }
    return 0;
}
static int32_t UiWrapSigned(int32_t v,int32_t minv,int32_t maxv){
    int64_t span=(int64_t)maxv-(int64_t)minv,x;
    if(span<=0)return minv;
    x=((int64_t)v-(int64_t)minv)%span;if(x<0)x+=span;
    return (int32_t)((int64_t)minv+x);
}

static int UiEqValuesProperty(NyotaUiControlSpec *s,const char *rhs){
    NyotaVal v=Eval(rhs);uint32_t i;
    if(v.type!=TYPE_LIST||v.list_len>NYOTA_UI_EQ_MAX_BARS){OutError("EQBOX VALUES wymaga LIST do 64 INTEGER");return 0;}
    memset(s->eq_values,0,sizeof(s->eq_values));s->eq_value_count=v.list_len;
    for(i=0;i<v.list_len;i++){if(v.list_items[i].type!=TYPE_INT||v.list_items[i].i<0||v.list_items[i].i>1000000){OutError("EQBOX VALUES: elementy musza byc INTEGER 0..1000000");return 0;}s->eq_values[i]=(uint32_t)v.list_items[i].i;}
    return 1;
}
static int UiEqLabelsProperty(NyotaUiControlSpec *s,const char *rhs){
    NyotaVal v=Eval(rhs);uint32_t i;
    if(v.type!=TYPE_LIST||v.list_len>NYOTA_UI_EQ_MAX_BARS){OutError("EQBOX LABELS wymaga LIST do 64 STRING");return 0;}
    memset(s->eq_labels,0,sizeof(s->eq_labels));s->eq_label_count=v.list_len;
    for(i=0;i<v.list_len;i++){if(v.list_items[i].type!=TYPE_STR){OutError("EQBOX LABELS: elementy musza byc STRING");return 0;}NStrCopy(s->eq_labels[i],v.list_items[i].s,sizeof(s->eq_labels[i]));}
    return 1;
}
static int UiEqColorsProperty(NyotaUiControlSpec *s,const char *rhs){
    NyotaVal v=Eval(rhs);uint32_t i;
    if(v.type!=TYPE_LIST||v.list_len>NYOTA_UI_EQ_MAX_BARS){OutError("EQBOX BARCOLORS wymaga LIST do 64 kolorow");return 0;}
    memset(s->eq_bar_color_set,0,sizeof(s->eq_bar_color_set));s->eq_color_count=v.list_len;
    for(i=0;i<v.list_len;i++){if(!ValToColorValue(&v.list_items[i],&s->eq_bar_colors[i])||s->eq_bar_colors[i].mode==NYOTA_COLOR_BACKDROP){OutError("EQBOX BARCOLORS: nieprawidlowy kolor");return 0;}s->eq_bar_color_set[i]=1;}
    return 1;
}

static int UiClockIntList(const char *rhs,int32_t *out,uint32_t *count,uint32_t cap,const char *label){
    NyotaVal v=Eval(rhs);uint32_t i;
    if(v.type!=TYPE_LIST||v.list_len>cap){char e[180];NStrCopy(e,label,sizeof(e));NStrAppend(e," wymaga LIST",sizeof(e));OutError(e);return 0;}
    for(i=0;i<v.list_len;i++){if(v.list_items[i].type!=TYPE_INT){char e[180];NStrCopy(e,label,sizeof(e));NStrAppend(e," wymaga INTEGER",sizeof(e));OutError(e);return 0;}out[i]=v.list_items[i].i;}
    *count=v.list_len;return 1;
}
static int UiClockUIntList(const char *rhs,uint32_t *out,uint32_t *count,uint32_t cap,uint32_t maxv,const char *label){
    NyotaVal v=Eval(rhs);uint32_t i;
    if(v.type!=TYPE_LIST||v.list_len>cap){char e[180];NStrCopy(e,label,sizeof(e));NStrAppend(e," wymaga LIST",sizeof(e));OutError(e);return 0;}
    for(i=0;i<v.list_len;i++){if(v.list_items[i].type!=TYPE_INT||v.list_items[i].i<0||(uint32_t)v.list_items[i].i>maxv){char e[180];NStrCopy(e,label,sizeof(e));NStrAppend(e," ma nieprawidlowy element",sizeof(e));OutError(e);return 0;}out[i]=(uint32_t)v.list_items[i].i;}
    *count=v.list_len;return 1;
}
static int UiClockColorsProperty(NyotaUiControlSpec *s,const char *rhs){
    NyotaVal v=Eval(rhs);uint32_t i;
    if(v.type!=TYPE_LIST||v.list_len>NYOTA_UI_CLOCK_MAX_NEEDLES){OutError("CLOCK NEEDLECOLORS wymaga LIST do 8 kolorow");return 0;}
    for(i=0;i<v.list_len;i++)if(!ValToColorValue(&v.list_items[i],&s->clock_needle_color[i])||s->clock_needle_color[i].mode==NYOTA_COLOR_BACKDROP){OutError("CLOCK NEEDLECOLORS: nieprawidlowy kolor");return 0;}
    s->clock_color_count=v.list_len;return 1;
}
static int UiClockNeedleShapeName(const char *v,uint8_t *out){
    if(NStrEq(v,"LINE"))*out=NYOTA_UI_NEEDLE_LINE;
    else if(NStrEq(v,"TRIANGLE"))*out=NYOTA_UI_NEEDLE_TRIANGLE;
    else if(NStrEq(v,"ARROW"))*out=NYOTA_UI_NEEDLE_ARROW;
    else if(NStrEq(v,"DIAMOND"))*out=NYOTA_UI_NEEDLE_DIAMOND;
    else if(NStrEq(v,"PARALLELOGRAM"))*out=NYOTA_UI_NEEDLE_PARALLELOGRAM;
    else if(NStrEq(v,"BAR"))*out=NYOTA_UI_NEEDLE_BAR;
    else if(NStrEq(v,"DOUBLE"))*out=NYOTA_UI_NEEDLE_DOUBLE;
    else return 0;
    return 1;
}
static int UiClockShapesProperty(NyotaUiControlSpec *s,const char *rhs){
    NyotaVal v=Eval(rhs);uint32_t i;
    if(v.type!=TYPE_LIST||v.list_len>NYOTA_UI_CLOCK_MAX_NEEDLES){OutError("CLOCK NEEDLESHAPES wymaga LIST do 8 STRING");return 0;}
    for(i=0;i<v.list_len;i++){if(v.list_items[i].type!=TYPE_STR||!UiClockNeedleShapeName(v.list_items[i].s,&s->clock_needle_shape[i])){OutError("CLOCK NEEDLESHAPES: LINE/TRIANGLE/ARROW/DIAMOND/PARALLELOGRAM/BAR/DOUBLE");return 0;}}
    s->clock_shape_count=v.list_len;return 1;
}
static int UiClockZonesProperty(NyotaUiControlSpec *s,const char *rhs){
    NyotaVal v=Eval(rhs);uint32_t i;
    if(v.type!=TYPE_LIST||v.list_len>NYOTA_UI_CLOCK_MAX_ZONES){OutError("CLOCK ZONES wymaga LIST do 8 stref");return 0;}
    for(i=0;i<v.list_len;i++){
        NyotaVal *z=&v.list_items[i];
        if((z->type!=TYPE_LIST&&z->type!=TYPE_TUPLE)||z->list_len!=3||z->list_items[0].type!=TYPE_INT||z->list_items[1].type!=TYPE_INT||
           !ValToColorValue(&z->list_items[2],&s->zone_color[i])||s->zone_color[i].mode==NYOTA_COLOR_BACKDROP){OutError("CLOCK ZONES: kazda strefa to [MIN, MAX, COLOR]");return 0;}
        if(z->list_items[1].i<=z->list_items[0].i){OutError("CLOCK ZONES: MAX musi byc wieksze od MIN");return 0;}
        s->zone_min[i]=z->list_items[0].i;s->zone_max[i]=z->list_items[1].i;
    }
    s->zone_count=v.list_len;return 1;
}

static int UiControlPropertyAllowed(uint8_t k,const char *p){
    if(k==NYOTA_UI_CTRL_BUTTON){
        return NStrEq(p,"TEXT")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"CTEXT")||NStrEq(p,"BOLD")||NStrEq(p,"ITALIC")||NStrEq(p,"UNDERLINE")||NStrEq(p,"HALIGN")||NStrEq(p,"VALIGN")||NStrEq(p,"WRAP")||NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"PADX")||NStrEq(p,"PADY");
    }
    if(k==NYOTA_UI_CTRL_LABEL) return NStrEq(p,"TEXT")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"CTEXT")||NStrEq(p,"BOLD")||NStrEq(p,"ITALIC")||NStrEq(p,"UNDERLINE")||NStrEq(p,"HALIGN")||NStrEq(p,"VALIGN")||NStrEq(p,"WRAP")||NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"PADX")||NStrEq(p,"PADY");
    if(k==NYOTA_UI_CTRL_PANEL) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"CLIP")||NStrEq(p,"RADIUS")||NStrEq(p,"LAYOUT")||NStrEq(p,"GAP")||NStrEq(p,"LPADX")||NStrEq(p,"LPADY");
    if(k==NYOTA_UI_CTRL_DAREA) return UiControlPropertyAllowed(NYOTA_UI_CTRL_LABEL,p)||NStrEq(p,"SHAPE")||NStrEq(p,"ACCEPT")||NStrEq(p,"MULTI")||NStrEq(p,"BGOVER")||NStrEq(p,"CBORDEROVER")||NStrEq(p,"BWIDTHOVER")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"PADX")||NStrEq(p,"PADY");
    if(k==NYOTA_UI_CTRL_CBOX) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"CCHECK")||NStrEq(p,"CHECKED")||NStrEq(p,"CSYMBOL")||NStrEq(p,"ENABLED");
    if(k==NYOTA_UI_CTRL_RADIO) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"CCHECK")||NStrEq(p,"CHECKED")||NStrEq(p,"RGROUP")||NStrEq(p,"ENABLED");
    if(k==NYOTA_UI_CTRL_COMBO) return NStrEq(p,"ITEMS")||NStrEq(p,"SELECTED")||NStrEq(p,"BG")||NStrEq(p,"CTEXT")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"CARROW")||NStrEq(p,"BGDROP")||NStrEq(p,"CDROP")||NStrEq(p,"BGSELECT")||NStrEq(p,"CSELECT")||NStrEq(p,"HOVER")||NStrEq(p,"BGHOVER")||NStrEq(p,"CHOVER")||NStrEq(p,"MAXVISIBLE")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"PADX")||NStrEq(p,"PADY");
    if(k==NYOTA_UI_CTRL_SEP) return NStrEq(p,"CSEP")||NStrEq(p,"THICK")||NStrEq(p,"EFFECT")||NStrEq(p,"BG");
    if(k==NYOTA_UI_CTRL_TABS) return NStrEq(p,"SELECTED");
    if(k==NYOTA_UI_CTRL_TAB) return NStrEq(p,"TABBG")||NStrEq(p,"TABFONT")||NStrEq(p,"TABFSIZE")||NStrEq(p,"TABCTEXT")||NStrEq(p,"TABBOLD")||NStrEq(p,"TABITALIC")||NStrEq(p,"TABUNDERLINE")||NStrEq(p,"TABBORDER")||NStrEq(p,"TABCBORDER")||NStrEq(p,"TABBWIDTH")||NStrEq(p,"TABRADIUS")||NStrEq(p,"TABPADX")||NStrEq(p,"TABPADY")||NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"CLIP")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"LAYOUT")||NStrEq(p,"GAP")||NStrEq(p,"LPADX")||NStrEq(p,"LPADY");
    if(k==NYOTA_UI_CTRL_TAREA) return NStrEq(p,"TEXT")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"CTEXT")||NStrEq(p,"BOLD")||NStrEq(p,"ITALIC")||NStrEq(p,"UNDERLINE")||NStrEq(p,"WRAP")||NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"PADX")||NStrEq(p,"PADY")||NStrEq(p,"READONLY")||NStrEq(p,"CCARET");
    if(k==NYOTA_UI_CTRL_SBAR) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"ORIENTATION")||NStrEq(p,"MIN")||NStrEq(p,"MAX")||NStrEq(p,"VALUE")||NStrEq(p,"PAGE")||NStrEq(p,"STEP")||NStrEq(p,"THUMB")||NStrEq(p,"CTHUMB")||NStrEq(p,"CTHUMBOVER");
    if(k==NYOTA_UI_CTRL_PBAR) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ORIENTATION")||NStrEq(p,"MIN")||NStrEq(p,"MAX")||NStrEq(p,"VALUE")||NStrEq(p,"SHAPE")||NStrEq(p,"SEGMENTS")||NStrEq(p,"SPACING")||NStrEq(p,"CFILL")||NStrEq(p,"CEMPTY");
    if(k==NYOTA_UI_CTRL_SLIDER) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"ORIENTATION")||NStrEq(p,"MIN")||NStrEq(p,"MAX")||NStrEq(p,"VALUE")||NStrEq(p,"STEP")||NStrEq(p,"THUMB")||NStrEq(p,"THUMBSIZE")||NStrEq(p,"CTHUMB")||NStrEq(p,"CTHUMBOVER");
    if(k==NYOTA_UI_CTRL_STATBAR||k==NYOTA_UI_CTRL_TOOLBAR) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"CLIP")||NStrEq(p,"LAYOUT")||NStrEq(p,"GAP")||NStrEq(p,"LPADX")||NStrEq(p,"LPADY");
    if(k==NYOTA_UI_CTRL_EQBOX) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"CTEXT")||NStrEq(p,"BOLD")||NStrEq(p,"ORIENTATION")||NStrEq(p,"DIRECTION")||NStrEq(p,"MIN")||NStrEq(p,"MAX")||NStrEq(p,"BARS")||NStrEq(p,"VALUES")||NStrEq(p,"BARWIDTH")||NStrEq(p,"GAP")||NStrEq(p,"SEGMENTGAP")||NStrEq(p,"MINHEIGHT")||NStrEq(p,"MAXHEIGHT")||NStrEq(p,"BUILD")||NStrEq(p,"BARBG")||NStrEq(p,"BARFILL")||NStrEq(p,"BARCOLORS")||NStrEq(p,"GLOW")||NStrEq(p,"BLUR")||NStrEq(p,"LABELS")||NStrEq(p,"LABELPOS")||NStrEq(p,"SHOWLABELS")||NStrEq(p,"SHOWVALUES");
    if(k==NYOTA_UI_CTRL_TBOX) return NStrEq(p,"TEXT")||NStrEq(p,"PLACEHOLDER")||NStrEq(p,"PASSWORD")||NStrEq(p,"MAXLEN")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"CTEXT")||NStrEq(p,"BOLD")||NStrEq(p,"ITALIC")||NStrEq(p,"UNDERLINE")||NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"PADX")||NStrEq(p,"PADY")||NStrEq(p,"READONLY")||NStrEq(p,"CCARET");
    if(k==NYOTA_UI_CTRL_SPINBOX) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"CTEXT")||NStrEq(p,"BOLD")||NStrEq(p,"MIN")||NStrEq(p,"MAX")||NStrEq(p,"VALUE")||NStrEq(p,"STEP");
    if(k==NYOTA_UI_CTRL_LISTVIEW) return NStrEq(p,"ITEMS")||NStrEq(p,"SELECTED")||NStrEq(p,"MULTI")||NStrEq(p,"ROWHEIGHT")||NStrEq(p,"BG")||NStrEq(p,"CTEXT")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"BOLD")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"PADX")||NStrEq(p,"PADY")||NStrEq(p,"BGSELECT")||NStrEq(p,"CSELECT")||NStrEq(p,"BGHOVER")||NStrEq(p,"CHOVER");
    if(k==NYOTA_UI_CTRL_TREEVIEW) return UiControlPropertyAllowed(NYOTA_UI_CTRL_LISTVIEW,p)||NStrEq(p,"INDENT")||NStrEq(p,"SHOWLINES");
    if(k==NYOTA_UI_CTRL_SPLITTER) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"RADIUS")||NStrEq(p,"ENABLED")||NStrEq(p,"ORIENTATION")||NStrEq(p,"MIN")||NStrEq(p,"MAX")||NStrEq(p,"VALUE")||NStrEq(p,"STEP")||NStrEq(p,"THICK")||NStrEq(p,"CSEP");
    if(k==NYOTA_UI_CTRL_SCALE) return NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"ENABLED")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"CTEXT")||NStrEq(p,"BOLD")||NStrEq(p,"MIN")||NStrEq(p,"MAX")||NStrEq(p,"VALUE")||NStrEq(p,"STEP")||NStrEq(p,"SHAPE")||NStrEq(p,"RADIUS")||NStrEq(p,"STARTANGLE")||NStrEq(p,"ENDANGLE")||NStrEq(p,"WRAP")||NStrEq(p,"INTERACTIVE")||NStrEq(p,"TRACKWIDTH")||NStrEq(p,"TRACKFILL")||NStrEq(p,"TRACKGLOW")||NStrEq(p,"TRACKBLUR")||NStrEq(p,"TRACKSTYLE")||NStrEq(p,"TRACKGAP")||NStrEq(p,"THUMB")||NStrEq(p,"THUMBSIZE")||NStrEq(p,"THUMBWIDTH")||NStrEq(p,"THUMBFILL")||NStrEq(p,"THUMBBORDER")||NStrEq(p,"CTHUMBBORDER")||NStrEq(p,"THUMBBWIDTH")||NStrEq(p,"THUMBGLOW")||NStrEq(p,"THUMBBLUR")||NStrEq(p,"THUMBROTATE")||NStrEq(p,"THUMBALIGN")||NStrEq(p,"MAJORSTEP")||NStrEq(p,"MINORSTEP")||NStrEq(p,"TICKSTYLE")||NStrEq(p,"CTICK")||NStrEq(p,"CMINORTICK")||NStrEq(p,"TICKLEN")||NStrEq(p,"MINORTICKLEN")||NStrEq(p,"TICKWIDTH")||NStrEq(p,"TICKBLUR")||NStrEq(p,"SHOWLABELS")||NStrEq(p,"LABELSTEP")||NStrEq(p,"LABELOFFSET");
    if(k==NYOTA_UI_CTRL_CLOCK) return NStrEq(p,"TEXT")||NStrEq(p,"UNIT")||NStrEq(p,"BG")||NStrEq(p,"BORDER")||NStrEq(p,"CBORDER")||NStrEq(p,"BWIDTH")||NStrEq(p,"ENABLED")||NStrEq(p,"FONT")||NStrEq(p,"FSIZE")||NStrEq(p,"CTEXT")||NStrEq(p,"BOLD")||NStrEq(p,"ITALIC")||NStrEq(p,"MIN")||NStrEq(p,"MAX")||NStrEq(p,"VALUE")||NStrEq(p,"SHAPE")||NStrEq(p,"RADIUS")||NStrEq(p,"STARTANGLE")||NStrEq(p,"ENDANGLE")||NStrEq(p,"MAJORSTEP")||NStrEq(p,"MINORSTEP")||NStrEq(p,"TICKSTYLE")||NStrEq(p,"CTICK")||NStrEq(p,"CMINORTICK")||NStrEq(p,"TICKLEN")||NStrEq(p,"MINORTICKLEN")||NStrEq(p,"TICKWIDTH")||NStrEq(p,"TICKBLUR")||NStrEq(p,"SHOWLABELS")||NStrEq(p,"LABELSTEP")||NStrEq(p,"LABELOFFSET")||NStrEq(p,"NEEDLES")||NStrEq(p,"VALUES")||NStrEq(p,"NEEDLE")||NStrEq(p,"NEEDLECOLORS")||NStrEq(p,"NEEDLESHAPES")||NStrEq(p,"NEEDLEWIDTHS")||NStrEq(p,"NEEDLELENS")||NStrEq(p,"NEEDLEBLURS")||NStrEq(p,"NEEDLEMINS")||NStrEq(p,"NEEDLEMAXS")||NStrEq(p,"DIALBLUR")||NStrEq(p,"CENTERDOT")||NStrEq(p,"CCENTER")||NStrEq(p,"CENTERSIZE")||NStrEq(p,"CENTERBLUR")||NStrEq(p,"SHOWVALUE")||NStrEq(p,"ZONES");
    return 0;
}

static int UiApplyControlProperty(NyotaUiControl *ctl,const char *prop,const char *rhs){
    NyotaUiControlSpec *s=&ctl->spec;
    if(!UiControlPropertyAllowed(s->kind,prop)){OutError("NyotaUI CONFIG: wlasciwosc niedozwolona dla tej kontrolki");return 0;}
    if(NStrEq(prop,"TEXT")){NyotaVal v=Eval(rhs);if(v.type!=TYPE_STR){OutError("TEXT wymaga STRING");return 0;}NStrCopy(s->text,v.s,sizeof(s->text));}
    else if(NStrEq(prop,"FONT")){NyotaVal v=Eval(rhs);if(v.type!=TYPE_STR||!v.s[0]){OutError("FONT wymaga niepustego STRING");return 0;}NStrCopy(s->font,v.s,sizeof(s->font));}
    else if(NStrEq(prop,"FSIZE")){if(!UiParseUIntProperty(rhs,&s->font_size,1,128,"FSIZE"))return 0;}
    else if(NStrEq(prop,"CTEXT")){if(!UiParseColorProperty(rhs,&s->text_color,"CTEXT"))return 0;}
    else if(NStrEq(prop,"BOLD")){if(!UiParseBoolProperty(rhs,&s->bold,"BOLD"))return 0;}
    else if(NStrEq(prop,"ITALIC")){if(!UiParseBoolProperty(rhs,&s->italic,"ITALIC"))return 0;}
    else if(NStrEq(prop,"UNDERLINE")){if(!UiParseBoolProperty(rhs,&s->underline,"UNDERLINE"))return 0;}
    else if(NStrEq(prop,"WRAP")){if(!UiParseBoolProperty(rhs,&s->wrap,"WRAP"))return 0;}
    else if(NStrEq(prop,"ENABLED")){if(!UiParseBoolProperty(rhs,&s->enabled,"ENABLED"))return 0;}
    else if(NStrEq(prop,"PADX")){if(!UiParseUIntProperty(rhs,&s->pad_x,0,256,"PADX"))return 0;}
    else if(NStrEq(prop,"PADY")){if(!UiParseUIntProperty(rhs,&s->pad_y,0,256,"PADY"))return 0;}
    else if(NStrEq(prop,"HALIGN")){const char *v=NTrim(rhs);if(NStrEq(v,"LEFT"))s->halign=NYOTA_UI_ALIGN_LEFT;else if(NStrEq(v,"CENTER"))s->halign=NYOTA_UI_ALIGN_CENTER;else if(NStrEq(v,"RIGHT"))s->halign=NYOTA_UI_ALIGN_RIGHT;else{OutError("HALIGN wymaga LEFT/CENTER/RIGHT");return 0;}}
    else if(NStrEq(prop,"VALIGN")){const char *v=NTrim(rhs);if(NStrEq(v,"TOP"))s->valign=NYOTA_UI_VALIGN_TOP;else if(NStrEq(v,"MIDDLE"))s->valign=NYOTA_UI_VALIGN_MIDDLE;else if(NStrEq(v,"BOTTOM"))s->valign=NYOTA_UI_VALIGN_BOTTOM;else{OutError("VALIGN wymaga TOP/MIDDLE/BOTTOM");return 0;}}
    else if(NStrEq(prop,"BG")){if(!UiParseBackground(rhs,&s->background))return 0;}
    else if(NStrEq(prop,"BORDER")){if(!UiParseBoolProperty(rhs,&s->border,"BORDER"))return 0;}
    else if(NStrEq(prop,"CBORDER")){if(!UiParseColorProperty(rhs,&s->border_color,"CBORDER"))return 0;}
    else if(NStrEq(prop,"BWIDTH")){if(!UiParseUIntProperty(rhs,&s->border_width,1,64,"BWIDTH"))return 0;}
    else if(NStrEq(prop,"CLIP")){if(!UiParseBoolProperty(rhs,&s->clip,"CLIP"))return 0;}
    else if(NStrEq(prop,"RADIUS")){
        if(s->kind==NYOTA_UI_CTRL_SCALE){if(!UiParseUIntProperty(rhs,&s->scale_radius,1,4096,"RADIUS"))return 0;}
        else if(s->kind==NYOTA_UI_CTRL_CLOCK){if(!UiParseUIntProperty(rhs,&s->clock_radius,1,4096,"RADIUS"))return 0;}
        else {if(!UiParseUIntProperty(rhs,&s->radius,0,1024,"RADIUS"))return 0;if(s->kind==NYOTA_UI_CTRL_DAREA&&s->shape!=NYOTA_UI_DAREA_RECT&&s->radius){OutError("DAREA RADIUS wymaga SHAPE=RECT");return 0;}}
    }
    else if(NStrEq(prop,"LAYOUT")){const char*v=NTrim(rhs);if(NStrEq(v,"FREE"))s->layout=NYOTA_UI_LAYOUT_FREE;else if(NStrEq(v,"ROW"))s->layout=NYOTA_UI_LAYOUT_ROW;else if(NStrEq(v,"COL"))s->layout=NYOTA_UI_LAYOUT_COL;else{OutError("LAYOUT wymaga FREE/ROW/COL");return 0;}}
    else if(NStrEq(prop,"GAP")){if(s->kind==NYOTA_UI_CTRL_EQBOX){if(!UiParseUIntProperty(rhs,&s->eq_gap,0,1024,"GAP"))return 0;}else if(!UiParseUIntProperty(rhs,&s->gap,0,1024,"GAP"))return 0;}
    else if(NStrEq(prop,"LPADX")){if(!UiParseUIntProperty(rhs,&s->layout_pad_x,0,1024,"LPADX"))return 0;}
    else if(NStrEq(prop,"LPADY")){if(!UiParseUIntProperty(rhs,&s->layout_pad_y,0,1024,"LPADY"))return 0;}
    else if(NStrEq(prop,"READONLY")){if(!UiParseBoolProperty(rhs,&s->readonly,"READONLY"))return 0;}
    else if(NStrEq(prop,"CCARET")){if(!UiParseColorProperty(rhs,&s->caret_color,"CCARET"))return 0;}
    else if(NStrEq(prop,"SHAPE")){
        const char*v=NTrim(rhs);
        if(s->kind==NYOTA_UI_CTRL_PBAR){
            if(NStrEq(v,"BAR"))s->progress_shape=NYOTA_UI_PBAR_SHAPE_BAR;
            else if(NStrEq(v,"CIRCLE"))s->progress_shape=NYOTA_UI_PBAR_SHAPE_CIRCLE;
            else if(NStrEq(v,"TRIANGLE"))s->progress_shape=NYOTA_UI_PBAR_SHAPE_TRIANGLE;
            else if(NStrEq(v,"SQUARE"))s->progress_shape=NYOTA_UI_PBAR_SHAPE_SQUARE;
            else if(NStrEq(v,"PARALLELOGRAM"))s->progress_shape=NYOTA_UI_PBAR_SHAPE_PARALLELOGRAM;
            else{OutError("PBAR SHAPE wymaga BAR/CIRCLE/TRIANGLE/SQUARE/PARALLELOGRAM");return 0;}
        }else if(s->kind==NYOTA_UI_CTRL_SCALE){
            if(NStrEq(v,"LINE"))s->scale_shape=NYOTA_UI_SCALE_LINE;
            else if(NStrEq(v,"ARC"))s->scale_shape=NYOTA_UI_SCALE_ARC;
            else if(NStrEq(v,"CIRCLE"))s->scale_shape=NYOTA_UI_SCALE_CIRCLE;
            else{OutError("SCALE SHAPE wymaga LINE/ARC/CIRCLE");return 0;}
        }else if(s->kind==NYOTA_UI_CTRL_CLOCK){
            if(NStrEq(v,"ARC"))s->clock_shape=NYOTA_UI_CLOCK_ARC;
            else if(NStrEq(v,"CIRCLE"))s->clock_shape=NYOTA_UI_CLOCK_CIRCLE;
            else{OutError("CLOCK SHAPE wymaga ARC/CIRCLE");return 0;}
        }else{
            if(NStrEq(v,"RECT"))s->shape=NYOTA_UI_DAREA_RECT;
            else if(NStrEq(v,"CIRCLE"))s->shape=NYOTA_UI_DAREA_CIRCLE;
            else if(NStrEq(v,"ELLIPSE"))s->shape=NYOTA_UI_DAREA_ELLIPSE;
            else{OutError("DAREA SHAPE wymaga RECT/CIRCLE/ELLIPSE");return 0;}
            if(s->shape!=NYOTA_UI_DAREA_RECT&&s->radius){OutError("DAREA RADIUS wymaga SHAPE=RECT");return 0;}
        }
    }
    else if(NStrEq(prop,"ACCEPT")){const char*v=NTrim(rhs);if(NStrEq(v,"FILES"))s->accept=NYOTA_UI_ACCEPT_FILES;else if(NStrEq(v,"DIRS"))s->accept=NYOTA_UI_ACCEPT_DIRS;else if(NStrEq(v,"ALL"))s->accept=NYOTA_UI_ACCEPT_ALL;else{OutError("DAREA ACCEPT wymaga FILES/DIRS/ALL");return 0;}}
    else if(NStrEq(prop,"MULTI")){if(!UiParseBoolProperty(rhs,&s->multi,"MULTI"))return 0;}
    else if(NStrEq(prop,"BGOVER")){if(!UiParseBackground(rhs,&s->background_over))return 0;}
    else if(NStrEq(prop,"CBORDEROVER")){if(!UiParseColorProperty(rhs,&s->border_color_over,"CBORDEROVER"))return 0;}
    else if(NStrEq(prop,"BWIDTHOVER")){if(!UiParseUIntProperty(rhs,&s->border_width_over,1,64,"BWIDTHOVER"))return 0;}
    else if(NStrEq(prop,"CHECKED")){if(!UiParseBoolProperty(rhs,&s->checked,"CHECKED"))return 0;}
    else if(NStrEq(prop,"CCHECK")){if(!UiParseColorProperty(rhs,&s->check_color,"CCHECK"))return 0;}
    else if(NStrEq(prop,"CSYMBOL")){const char*v=NTrim(rhs);if(NStrEq(v,"X"))NStrCopy(s->check_symbol,"X",sizeof(s->check_symbol));else{NyotaVal q=Eval(rhs);if(q.type!=TYPE_STR||!q.s[0]){OutError("CSYMBOL wymaga X albo niepustego STRING");return 0;}NStrCopy(s->check_symbol,q.s,sizeof(s->check_symbol));}}
    else if(NStrEq(prop,"RGROUP")){char n[64];uint32_t z=ParseIdent(NTrim(rhs),n,sizeof(n));if(!n[0]||*NTrim(NTrim(rhs)+z)){OutError("RGROUP wymaga identyfikatora grupy");return 0;}NStrCopy(s->radio_group,n,sizeof(s->radio_group));}
    else if(NStrEq(prop,"ITEMS")){NyotaVal v=Eval(rhs);if(!UiListToItems(&v,s->items,sizeof(s->items)))return 0;if(s->selected>=UiItemsCount(s->items)&&UiItemsCount(s->items))s->selected=0;}
    else if(NStrEq(prop,"SELECTED")){if(!UiParseUIntProperty(rhs,&s->selected,0,65535,"SELECTED"))return 0;if(s->kind==NYOTA_UI_CTRL_COMBO&&UiItemsCount(s->items)&&s->selected>=UiItemsCount(s->items)){OutError("COMBO SELECTED poza ITEMS");return 0;}}
    else if(NStrEq(prop,"CARROW")){if(!UiParseColorProperty(rhs,&s->arrow_color,"CARROW"))return 0;}
    else if(NStrEq(prop,"BGDROP")){if(!UiParseBackground(rhs,&s->drop_background))return 0;}
    else if(NStrEq(prop,"CDROP")){if(!UiParseColorProperty(rhs,&s->drop_text_color,"CDROP"))return 0;}
    else if(NStrEq(prop,"BGSELECT")){if(!UiParseBackground(rhs,&s->selected_background))return 0;}
    else if(NStrEq(prop,"CSELECT")){if(!UiParseColorProperty(rhs,&s->selected_text_color,"CSELECT"))return 0;}
    else if(NStrEq(prop,"HOVER")){const char*v=NTrim(rhs);if(NStrEq(v,"ON"))s->hover_enabled=1;else if(NStrEq(v,"OFF"))s->hover_enabled=0;else{OutError("HOVER wymaga ON/OFF");return 0;}}
    else if(NStrEq(prop,"BGHOVER")){if(!UiParseBackground(rhs,&s->hover_background))return 0;}
    else if(NStrEq(prop,"CHOVER")){if(!UiParseColorProperty(rhs,&s->hover_text_color,"CHOVER"))return 0;}
    else if(NStrEq(prop,"MAXVISIBLE")){if(!UiParseUIntProperty(rhs,&s->max_visible,1,64,"MAXVISIBLE"))return 0;}
    else if(NStrEq(prop,"CSEP")){if(!UiParseColorProperty(rhs,&s->sep_color,"CSEP"))return 0;}
    else if(NStrEq(prop,"THICK")){if(s->kind==NYOTA_UI_CTRL_SPLITTER){if(!UiParseUIntProperty(rhs,&s->splitter_thickness,1,128,"THICK"))return 0;}else if(!UiParseUIntProperty(rhs,&s->sep_thickness,1,64,"THICK"))return 0;}
    else if(NStrEq(prop,"EFFECT")){const char*v=NTrim(rhs);if(NStrEq(v,"NORMAL"))s->sep_effect=NYOTA_UI_SEP_NORMAL;else if(NStrEq(v,"INSET"))s->sep_effect=NYOTA_UI_SEP_INSET;else if(NStrEq(v,"RAISED"))s->sep_effect=NYOTA_UI_SEP_RAISED;else if(NStrEq(v,"GRADIENT"))s->sep_effect=NYOTA_UI_SEP_GRADIENT;else{OutError("SEP EFFECT wymaga NORMAL/INSET/RAISED/GRADIENT");return 0;}}
    else if(NStrEq(prop,"ORIENTATION")){const char*v=NTrim(rhs);if(NStrEq(v,"HORIZONTAL"))s->orientation=NYOTA_UI_SEP_HORIZONTAL;else if(NStrEq(v,"VERTICAL"))s->orientation=NYOTA_UI_SEP_VERTICAL;else{OutError("ORIENTATION wymaga HORIZONTAL/VERTICAL");return 0;}}
    else if(NStrEq(prop,"MIN")){
        if(s->kind==NYOTA_UI_CTRL_SPINBOX||s->kind==NYOTA_UI_CTRL_SCALE||s->kind==NYOTA_UI_CTRL_CLOCK){
            uint32_t i;if(!UiParseIntProperty(rhs,&s->signed_min,-1000000,1000000,"MIN"))return 0;
            if(s->kind==NYOTA_UI_CTRL_CLOCK)for(i=0;i<NYOTA_UI_CLOCK_MAX_NEEDLES;i++)s->clock_needle_min[i]=s->signed_min;
        }else if(!UiParseUIntProperty(rhs,&s->range_min,0,1000000,"MIN"))return 0;
    }
    else if(NStrEq(prop,"MAX")){
        if(s->kind==NYOTA_UI_CTRL_SPINBOX||s->kind==NYOTA_UI_CTRL_SCALE||s->kind==NYOTA_UI_CTRL_CLOCK){
            uint32_t i;if(!UiParseIntProperty(rhs,&s->signed_max,-1000000,1000000,"MAX"))return 0;
            if(s->kind==NYOTA_UI_CTRL_CLOCK)for(i=0;i<NYOTA_UI_CLOCK_MAX_NEEDLES;i++)s->clock_needle_max[i]=s->signed_max;
        }else if(!UiParseUIntProperty(rhs,&s->range_max,0,1000000,"MAX"))return 0;
    }
    else if(NStrEq(prop,"VALUE")){
        if(s->kind==NYOTA_UI_CTRL_SPINBOX||s->kind==NYOTA_UI_CTRL_SCALE||s->kind==NYOTA_UI_CTRL_CLOCK){
            if(!UiParseIntProperty(rhs,&s->signed_value,-1000000,1000000,"VALUE"))return 0;
            if(s->kind==NYOTA_UI_CTRL_CLOCK){s->clock_values[0]=s->signed_value;s->clock_value_count=1;}
        }else if(!UiParseUIntProperty(rhs,&s->range_value,0,1000000,"VALUE"))return 0;
    }
    else if(NStrEq(prop,"PAGE")){if(!UiParseUIntProperty(rhs,&s->range_page,1,1000000,"PAGE"))return 0;}
    else if(NStrEq(prop,"STEP")){
        if(s->kind==NYOTA_UI_CTRL_SPINBOX||s->kind==NYOTA_UI_CTRL_SCALE){if(!UiParseIntProperty(rhs,&s->signed_step,1,1000000,"STEP"))return 0;}
        else if(!UiParseUIntProperty(rhs,&s->range_step,1,1000000,"STEP"))return 0;
    }
    else if(NStrEq(prop,"THUMB")){const char*v=NTrim(rhs);
        if(s->kind==NYOTA_UI_CTRL_SCALE){
            if(NStrEq(v,"RECT"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_RECT;else if(NStrEq(v,"ROUND"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_ROUND;else if(NStrEq(v,"CIRCLE"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_CIRCLE;else if(NStrEq(v,"DIAMOND"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_DIAMOND;else if(NStrEq(v,"TRIANGLE"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_TRIANGLE;else if(NStrEq(v,"PARALLELOGRAM"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_PARALLELOGRAM;else if(NStrEq(v,"LINE"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_LINE;else if(NStrEq(v,"NEEDLE"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_NEEDLE;else if(NStrEq(v,"DOT"))s->scale_thumb_shape=NYOTA_UI_SCALE_THUMB_DOT;else{OutError("SCALE THUMB: RECT/ROUND/CIRCLE/DIAMOND/TRIANGLE/PARALLELOGRAM/LINE/NEEDLE/DOT");return 0;}
        }else{
            if(NStrEq(v,"RECT"))s->thumb_shape=NYOTA_UI_SBAR_THUMB_RECT;else if(NStrEq(v,"ROUND"))s->thumb_shape=NYOTA_UI_SBAR_THUMB_ROUND;else if(NStrEq(v,"CIRCLE"))s->thumb_shape=NYOTA_UI_SBAR_THUMB_CIRCLE;else if(NStrEq(v,"DIAMOND"))s->thumb_shape=NYOTA_UI_SBAR_THUMB_DIAMOND;else if(NStrEq(v,"TRIANGLE"))s->thumb_shape=NYOTA_UI_SBAR_THUMB_TRIANGLE;else if(NStrEq(v,"PARALLELOGRAM"))s->thumb_shape=NYOTA_UI_SBAR_THUMB_PARALLELOGRAM;else{OutError("SBAR THUMB wymaga RECT/ROUND/CIRCLE/DIAMOND/TRIANGLE/PARALLELOGRAM");return 0;}
        }
    }
    else if(NStrEq(prop,"CTHUMB")){if(!UiParseColorProperty(rhs,&s->thumb_color,"CTHUMB"))return 0;}
    else if(NStrEq(prop,"CTHUMBOVER")){if(!UiParseColorProperty(rhs,&s->thumb_hover_color,"CTHUMBOVER"))return 0;}
    else if(NStrEq(prop,"SEGMENTS")){if(!UiParseUIntProperty(rhs,&s->progress_segments,1,256,"SEGMENTS"))return 0;}
    else if(NStrEq(prop,"SPACING")){if(!UiParseUIntProperty(rhs,&s->progress_gap,0,128,"SPACING"))return 0;}
    else if(NStrEq(prop,"CFILL")){if(!UiParseColorProperty(rhs,&s->progress_fill_color,"CFILL"))return 0;}
    else if(NStrEq(prop,"CEMPTY")){if(!UiParseColorProperty(rhs,&s->progress_empty_color,"CEMPTY"))return 0;}
    else if(NStrEq(prop,"THUMBSIZE")){if(s->kind==NYOTA_UI_CTRL_SCALE){if(!UiParseUIntProperty(rhs,&s->scale_thumb_size,1,512,"THUMBSIZE"))return 0;}else if(!UiParseUIntProperty(rhs,&s->slider_thumb_size,4,256,"THUMBSIZE"))return 0;}
    else if(NStrEq(prop,"BARS")){if(!UiParseUIntProperty(rhs,&s->eq_bars,1,NYOTA_UI_EQ_MAX_BARS,"BARS"))return 0;}
    else if(NStrEq(prop,"VALUES")){if(s->kind==NYOTA_UI_CTRL_CLOCK){if(!UiClockIntList(rhs,s->clock_values,&s->clock_value_count,NYOTA_UI_CLOCK_MAX_NEEDLES,"CLOCK VALUES"))return 0;}else if(!UiEqValuesProperty(s,rhs))return 0;}
    else if(NStrEq(prop,"BARWIDTH")){if(!UiParseUIntProperty(rhs,&s->eq_bar_width,1,1024,"BARWIDTH"))return 0;}
    else if(NStrEq(prop,"SEGMENTGAP")){if(!UiParseUIntProperty(rhs,&s->eq_segment_gap,0,128,"SEGMENTGAP"))return 0;}
    else if(NStrEq(prop,"MINHEIGHT")){if(!UiParseUIntProperty(rhs,&s->eq_min_height,0,4096,"MINHEIGHT"))return 0;}
    else if(NStrEq(prop,"MAXHEIGHT")){if(!UiParseUIntProperty(rhs,&s->eq_max_height,0,4096,"MAXHEIGHT"))return 0;}
    else if(NStrEq(prop,"BUILD")){const char*v=NTrim(rhs);if(NStrEq(v,"SOLID"))s->eq_build=NYOTA_UI_EQ_SOLID;else if(NStrEq(v,"CIRCLE"))s->eq_build=NYOTA_UI_EQ_CIRCLE;else if(NStrEq(v,"SQUARE"))s->eq_build=NYOTA_UI_EQ_SQUARE;else if(NStrEq(v,"TRIANGLE"))s->eq_build=NYOTA_UI_EQ_TRIANGLE;else if(NStrEq(v,"DIAMOND"))s->eq_build=NYOTA_UI_EQ_DIAMOND;else if(NStrEq(v,"PARALLELOGRAM"))s->eq_build=NYOTA_UI_EQ_PARALLELOGRAM;else{OutError("EQBOX BUILD wymaga SOLID/CIRCLE/SQUARE/TRIANGLE/DIAMOND/PARALLELOGRAM");return 0;}}
    else if(NStrEq(prop,"DIRECTION")){const char*v=NTrim(rhs);if(NStrEq(v,"UP"))s->eq_direction=NYOTA_UI_EQ_UP;else if(NStrEq(v,"DOWN"))s->eq_direction=NYOTA_UI_EQ_DOWN;else if(NStrEq(v,"LEFT"))s->eq_direction=NYOTA_UI_EQ_LEFT;else if(NStrEq(v,"RIGHT"))s->eq_direction=NYOTA_UI_EQ_RIGHT;else if(NStrEq(v,"CENTER"))s->eq_direction=NYOTA_UI_EQ_CENTER;else{OutError("EQBOX DIRECTION wymaga UP/DOWN/LEFT/RIGHT/CENTER");return 0;}}
    else if(NStrEq(prop,"BARBG")){if(!UiParseBackground(rhs,&s->eq_bar_background))return 0;}
    else if(NStrEq(prop,"BARFILL")){if(!UiParseBackground(rhs,&s->eq_bar_fill))return 0;}
    else if(NStrEq(prop,"BARCOLORS")){if(!UiEqColorsProperty(s,rhs))return 0;}
    else if(NStrEq(prop,"GLOW")){if(!UiParseBackground(rhs,&s->eq_glow))return 0;}
    else if(NStrEq(prop,"BLUR")){if(!UiParseUIntProperty(rhs,&s->eq_blur,0,64,"BLUR"))return 0;}
    else if(NStrEq(prop,"LABELS")){if(!UiEqLabelsProperty(s,rhs))return 0;}
    else if(NStrEq(prop,"LABELPOS")){const char*v=NTrim(rhs);if(NStrEq(v,"INSIDE"))s->eq_label_pos=NYOTA_UI_EQ_LABEL_INSIDE;else if(NStrEq(v,"BOTTOM"))s->eq_label_pos=NYOTA_UI_EQ_LABEL_BOTTOM;else if(NStrEq(v,"TOP"))s->eq_label_pos=NYOTA_UI_EQ_LABEL_TOP;else{OutError("EQBOX LABELPOS wymaga INSIDE/BOTTOM/TOP");return 0;}}
    else if(NStrEq(prop,"SHOWLABELS")){if(s->kind==NYOTA_UI_CTRL_EQBOX){if(!UiParseBoolProperty(rhs,&s->eq_show_labels,"SHOWLABELS"))return 0;}else if(!UiParseBoolProperty(rhs,&s->show_labels,"SHOWLABELS"))return 0;}
    else if(NStrEq(prop,"SHOWVALUES")){if(!UiParseBoolProperty(rhs,&s->eq_show_values,"SHOWVALUES"))return 0;}
    else if(NStrEq(prop,"PLACEHOLDER")){if(!UiParseStringProperty(rhs,s->placeholder,sizeof(s->placeholder),"PLACEHOLDER"))return 0;}
    else if(NStrEq(prop,"PASSWORD")){if(!UiParseBoolProperty(rhs,&s->password,"PASSWORD"))return 0;}
    else if(NStrEq(prop,"MAXLEN")){if(!UiParseUIntProperty(rhs,&s->max_length,1,NYOTA_UI_TEXT_MAX-1u,"MAXLEN"))return 0;}
    else if(NStrEq(prop,"ROWHEIGHT")){if(!UiParseUIntProperty(rhs,&s->row_height,12,256,"ROWHEIGHT"))return 0;}
    else if(NStrEq(prop,"INDENT")){if(!UiParseUIntProperty(rhs,&s->tree_indent,4,256,"INDENT"))return 0;}
    else if(NStrEq(prop,"SHOWLINES")){if(!UiParseBoolProperty(rhs,&s->show_lines,"SHOWLINES"))return 0;}
    else if(NStrEq(prop,"STARTANGLE")){if(!UiParseIntProperty(rhs,&s->start_angle,-3600,3600,"STARTANGLE"))return 0;}
    else if(NStrEq(prop,"ENDANGLE")){if(!UiParseIntProperty(rhs,&s->end_angle,-3600,3600,"ENDANGLE"))return 0;}
    else if(NStrEq(prop,"WRAP")){if(!UiParseBoolProperty(rhs,&s->scale_wrap,"WRAP"))return 0;}
    else if(NStrEq(prop,"INTERACTIVE")){if(!UiParseBoolProperty(rhs,&s->scale_interactive,"INTERACTIVE"))return 0;}
    else if(NStrEq(prop,"TRACKWIDTH")){if(!UiParseUIntProperty(rhs,&s->track_width,1,128,"TRACKWIDTH"))return 0;}
    else if(NStrEq(prop,"TRACKFILL")){if(!UiParseBackground(rhs,&s->track_fill))return 0;}
    else if(NStrEq(prop,"TRACKGLOW")){if(!UiParseBackground(rhs,&s->track_glow))return 0;}
    else if(NStrEq(prop,"TRACKBLUR")){if(!UiParseUIntProperty(rhs,&s->track_blur,0,64,"TRACKBLUR"))return 0;}
    else if(NStrEq(prop,"TRACKGAP")){if(!UiParseUIntProperty(rhs,&s->track_gap,0,256,"TRACKGAP"))return 0;}
    else if(NStrEq(prop,"TRACKSTYLE")){const char*v=NTrim(rhs);if(NStrEq(v,"SOLID"))s->track_style=NYOTA_UI_TRACK_SOLID;else if(NStrEq(v,"DASH"))s->track_style=NYOTA_UI_TRACK_DASH;else if(NStrEq(v,"DOT"))s->track_style=NYOTA_UI_TRACK_DOT;else if(NStrEq(v,"SEGMENT"))s->track_style=NYOTA_UI_TRACK_SEGMENT;else{OutError("TRACKSTYLE wymaga SOLID/DASH/DOT/SEGMENT");return 0;}}
    else if(NStrEq(prop,"THUMBWIDTH")){if(!UiParseUIntProperty(rhs,&s->scale_thumb_width,1,128,"THUMBWIDTH"))return 0;}
    else if(NStrEq(prop,"THUMBFILL")){if(!UiParseBackground(rhs,&s->scale_thumb_fill))return 0;}
    else if(NStrEq(prop,"THUMBBORDER")){if(!UiParseBoolProperty(rhs,&s->scale_thumb_border,"THUMBBORDER"))return 0;}
    else if(NStrEq(prop,"CTHUMBBORDER")){if(!UiParseColorProperty(rhs,&s->scale_thumb_border_color,"CTHUMBBORDER"))return 0;}
    else if(NStrEq(prop,"THUMBBWIDTH")){if(!UiParseUIntProperty(rhs,&s->scale_thumb_border_width,1,64,"THUMBBWIDTH"))return 0;}
    else if(NStrEq(prop,"THUMBGLOW")){if(!UiParseBackground(rhs,&s->scale_thumb_glow))return 0;}
    else if(NStrEq(prop,"THUMBBLUR")){if(!UiParseUIntProperty(rhs,&s->scale_thumb_blur,0,64,"THUMBBLUR"))return 0;}
    else if(NStrEq(prop,"THUMBROTATE")){if(!UiParseBoolProperty(rhs,&s->thumb_rotate,"THUMBROTATE"))return 0;}
    else if(NStrEq(prop,"THUMBALIGN")){const char*v=NTrim(rhs);if(NStrEq(v,"FIXED"))s->thumb_align=NYOTA_UI_THUMB_ALIGN_FIXED;else if(NStrEq(v,"RADIAL"))s->thumb_align=NYOTA_UI_THUMB_ALIGN_RADIAL;else if(NStrEq(v,"TANGENT"))s->thumb_align=NYOTA_UI_THUMB_ALIGN_TANGENT;else{OutError("THUMBALIGN wymaga FIXED/RADIAL/TANGENT");return 0;}}
    else if(NStrEq(prop,"MAJORSTEP")){if(!UiParseIntProperty(rhs,&s->major_step,1,1000000,"MAJORSTEP"))return 0;}
    else if(NStrEq(prop,"MINORSTEP")){if(!UiParseIntProperty(rhs,&s->minor_step,1,1000000,"MINORSTEP"))return 0;}
    else if(NStrEq(prop,"TICKSTYLE")){const char*v=NTrim(rhs);if(NStrEq(v,"LINE"))s->tick_style=NYOTA_UI_TICK_LINE;else if(NStrEq(v,"DOT"))s->tick_style=NYOTA_UI_TICK_DOT;else if(NStrEq(v,"RECT"))s->tick_style=NYOTA_UI_TICK_RECT;else if(NStrEq(v,"ROUND"))s->tick_style=NYOTA_UI_TICK_ROUND;else if(NStrEq(v,"TRIANGLE"))s->tick_style=NYOTA_UI_TICK_TRIANGLE;else if(NStrEq(v,"DIAMOND"))s->tick_style=NYOTA_UI_TICK_DIAMOND;else{OutError("TICKSTYLE wymaga LINE/DOT/RECT/ROUND/TRIANGLE/DIAMOND");return 0;}}
    else if(NStrEq(prop,"CTICK")){if(!UiParseColorProperty(rhs,&s->tick_color,"CTICK"))return 0;}
    else if(NStrEq(prop,"CMINORTICK")){if(!UiParseColorProperty(rhs,&s->minor_tick_color,"CMINORTICK"))return 0;}
    else if(NStrEq(prop,"TICKLEN")){if(!UiParseUIntProperty(rhs,&s->tick_len,1,256,"TICKLEN"))return 0;}
    else if(NStrEq(prop,"MINORTICKLEN")){if(!UiParseUIntProperty(rhs,&s->minor_tick_len,1,256,"MINORTICKLEN"))return 0;}
    else if(NStrEq(prop,"TICKWIDTH")){if(!UiParseUIntProperty(rhs,&s->tick_width,1,64,"TICKWIDTH"))return 0;}
    else if(NStrEq(prop,"TICKBLUR")){if(!UiParseUIntProperty(rhs,&s->tick_blur,0,64,"TICKBLUR"))return 0;}
    else if(NStrEq(prop,"LABELSTEP")){if(!UiParseIntProperty(rhs,&s->label_step,1,1000000,"LABELSTEP"))return 0;}
    else if(NStrEq(prop,"LABELOFFSET")){if(!UiParseIntProperty(rhs,&s->label_offset,-1024,1024,"LABELOFFSET"))return 0;}
    else if(NStrEq(prop,"NEEDLES")){if(!UiParseUIntProperty(rhs,&s->clock_needles,1,NYOTA_UI_CLOCK_MAX_NEEDLES,"NEEDLES"))return 0;}
    else if(NStrEq(prop,"NEEDLE")){uint8_t sh;const char*v=NTrim(rhs);uint32_t i;if(!UiClockNeedleShapeName(v,&sh)){OutError("NEEDLE: LINE/TRIANGLE/ARROW/DIAMOND/PARALLELOGRAM/BAR/DOUBLE");return 0;}for(i=0;i<NYOTA_UI_CLOCK_MAX_NEEDLES;i++)s->clock_needle_shape[i]=sh;s->clock_shape_count=s->clock_needles;}
    else if(NStrEq(prop,"NEEDLECOLORS")){if(!UiClockColorsProperty(s,rhs))return 0;}
    else if(NStrEq(prop,"NEEDLESHAPES")){if(!UiClockShapesProperty(s,rhs))return 0;}
    else if(NStrEq(prop,"NEEDLEWIDTHS")){if(!UiClockUIntList(rhs,s->clock_needle_width,&s->clock_width_count,NYOTA_UI_CLOCK_MAX_NEEDLES,128,"NEEDLEWIDTHS"))return 0;}
    else if(NStrEq(prop,"NEEDLELENS")){if(!UiClockUIntList(rhs,s->clock_needle_len,&s->clock_len_count,NYOTA_UI_CLOCK_MAX_NEEDLES,4096,"NEEDLELENS"))return 0;}
    else if(NStrEq(prop,"NEEDLEBLURS")){if(!UiClockUIntList(rhs,s->clock_needle_blur,&s->clock_blur_count,NYOTA_UI_CLOCK_MAX_NEEDLES,64,"NEEDLEBLURS"))return 0;}
    else if(NStrEq(prop,"NEEDLEMINS")){if(!UiClockIntList(rhs,s->clock_needle_min,&s->clock_min_count,NYOTA_UI_CLOCK_MAX_NEEDLES,"NEEDLEMINS"))return 0;}
    else if(NStrEq(prop,"NEEDLEMAXS")){if(!UiClockIntList(rhs,s->clock_needle_max,&s->clock_max_count,NYOTA_UI_CLOCK_MAX_NEEDLES,"NEEDLEMAXS"))return 0;}
    else if(NStrEq(prop,"DIALBLUR")){if(!UiParseUIntProperty(rhs,&s->dial_blur,0,64,"DIALBLUR"))return 0;}
    else if(NStrEq(prop,"CENTERDOT")){if(!UiParseBoolProperty(rhs,&s->center_dot,"CENTERDOT"))return 0;}
    else if(NStrEq(prop,"CCENTER")){if(!UiParseColorProperty(rhs,&s->center_color,"CCENTER"))return 0;}
    else if(NStrEq(prop,"CENTERSIZE")){if(!UiParseUIntProperty(rhs,&s->center_size,1,256,"CENTERSIZE"))return 0;}
    else if(NStrEq(prop,"CENTERBLUR")){if(!UiParseUIntProperty(rhs,&s->center_blur,0,64,"CENTERBLUR"))return 0;}
    else if(NStrEq(prop,"SHOWVALUE")){if(!UiParseBoolProperty(rhs,&s->show_value,"SHOWVALUE"))return 0;}
    else if(NStrEq(prop,"UNIT")){if(!UiParseStringProperty(rhs,s->unit,sizeof(s->unit),"UNIT"))return 0;}
    else if(NStrEq(prop,"ZONES")){if(!UiClockZonesProperty(s,rhs))return 0;}
    else if(NStrEq(prop,"TABBG")){if(!UiParseBackground(rhs,&s->tab_background))return 0;}
    else if(NStrEq(prop,"TABFONT")){NyotaVal v=Eval(rhs);if(v.type!=TYPE_STR||!v.s[0]){OutError("TABFONT wymaga niepustego STRING");return 0;}NStrCopy(s->tab_font,v.s,sizeof(s->tab_font));}
    else if(NStrEq(prop,"TABFSIZE")){if(!UiParseUIntProperty(rhs,&s->tab_font_size,1,128,"TABFSIZE"))return 0;}
    else if(NStrEq(prop,"TABCTEXT")){if(!UiParseColorProperty(rhs,&s->tab_text_color,"TABCTEXT"))return 0;}
    else if(NStrEq(prop,"TABBOLD")){if(!UiParseBoolProperty(rhs,&s->tab_bold,"TABBOLD"))return 0;}
    else if(NStrEq(prop,"TABITALIC")){if(!UiParseBoolProperty(rhs,&s->tab_italic,"TABITALIC"))return 0;}
    else if(NStrEq(prop,"TABUNDERLINE")){if(!UiParseBoolProperty(rhs,&s->tab_underline,"TABUNDERLINE"))return 0;}
    else if(NStrEq(prop,"TABBORDER")){if(!UiParseBoolProperty(rhs,&s->tab_border,"TABBORDER"))return 0;}
    else if(NStrEq(prop,"TABCBORDER")){if(!UiParseColorProperty(rhs,&s->tab_border_color,"TABCBORDER"))return 0;}
    else if(NStrEq(prop,"TABBWIDTH")){if(!UiParseUIntProperty(rhs,&s->tab_border_width,1,64,"TABBWIDTH"))return 0;}
    else if(NStrEq(prop,"TABRADIUS")){if(!UiParseUIntProperty(rhs,&s->tab_radius,0,1024,"TABRADIUS"))return 0;}
    else if(NStrEq(prop,"TABPADX")){if(!UiParseUIntProperty(rhs,&s->tab_pad_x,0,256,"TABPADX"))return 0;}
    else if(NStrEq(prop,"TABPADY")){if(!UiParseUIntProperty(rhs,&s->tab_pad_y,0,256,"TABPADY"))return 0;}
    return 1;
}

static int UiValidateControlSpec(const NyotaUiControlSpec *s){
    if(s->position_mode==NYOTA_UI_POS_AUTO && s->kind==NYOTA_UI_CTRL_TAB){OutError("TAB nie przyjmuje pozycji AUTO");return 0;}
    if(s->kind==NYOTA_UI_CTRL_SEP&&(s->sep_effect==NYOTA_UI_SEP_INSET||s->sep_effect==NYOTA_UI_SEP_RAISED)&&s->sep_thickness<2){OutError("SEP INSET/RAISED wymaga THICK >= 2");return 0;}
    if(s->kind==NYOTA_UI_CTRL_DAREA&&s->shape!=NYOTA_UI_DAREA_RECT&&s->radius){OutError("DAREA RADIUS wymaga SHAPE=RECT");return 0;}
    if(s->kind==NYOTA_UI_CTRL_COMBO&&UiItemsCount(s->items)&&s->selected>=UiItemsCount(s->items)){OutError("COMBO SELECTED poza ITEMS");return 0;}
    if((s->kind==NYOTA_UI_CTRL_SBAR||s->kind==NYOTA_UI_CTRL_PBAR||s->kind==NYOTA_UI_CTRL_SLIDER||s->kind==NYOTA_UI_CTRL_EQBOX)&&s->range_max<=s->range_min){OutError("NyotaUI range: MAX musi byc wieksze od MIN");return 0;}
    if((s->kind==NYOTA_UI_CTRL_SBAR||s->kind==NYOTA_UI_CTRL_PBAR||s->kind==NYOTA_UI_CTRL_SLIDER)&&(s->range_value<s->range_min||s->range_value>s->range_max)){OutError("NyotaUI range: VALUE poza zakresem MIN..MAX");return 0;}
    if(s->kind==NYOTA_UI_CTRL_EQBOX){
        uint32_t i;
        if(!s->eq_bars||s->eq_bars>NYOTA_UI_EQ_MAX_BARS){OutError("EQBOX BARS poza zakresem 1..64");return 0;}
        if(s->eq_value_count>s->eq_bars||s->eq_label_count>s->eq_bars||s->eq_color_count>s->eq_bars){OutError("EQBOX: VALUES/LABELS/BARCOLORS nie moga miec wiecej elementow niz BARS");return 0;}
        if(s->eq_max_height&&s->eq_min_height>s->eq_max_height){OutError("EQBOX: MINHEIGHT nie moze przekraczac MAXHEIGHT");return 0;}
        if(s->orientation==NYOTA_UI_SEP_VERTICAL&&(s->eq_direction==NYOTA_UI_EQ_LEFT||s->eq_direction==NYOTA_UI_EQ_RIGHT)){OutError("EQBOX VERTICAL wymaga DIRECTION=UP/DOWN/CENTER");return 0;}
        if(s->orientation==NYOTA_UI_SEP_HORIZONTAL&&(s->eq_direction==NYOTA_UI_EQ_UP||s->eq_direction==NYOTA_UI_EQ_DOWN)){OutError("EQBOX HORIZONTAL wymaga DIRECTION=LEFT/RIGHT/CENTER");return 0;}
        for(i=0;i<s->eq_value_count;i++)if(s->eq_values[i]<s->range_min||s->eq_values[i]>s->range_max){OutError("EQBOX VALUES: wartosc poza MIN..MAX");return 0;}
    }
    if(s->kind==NYOTA_UI_CTRL_TBOX){
        if(strlen(s->text)>s->max_length){OutError("TBOX: TEXT przekracza MAXLEN");return 0;}
        if(strchr(s->text,'\n')||strchr(s->text,'\r')){OutError("TBOX: TEXT musi byc jednowierszowy");return 0;}
    }
    if(s->kind==NYOTA_UI_CTRL_SPINBOX||s->kind==NYOTA_UI_CTRL_SCALE){
        if(s->signed_max<=s->signed_min){OutError("SPINBOX/SCALE: MAX musi byc wieksze od MIN");return 0;}
        if(s->signed_step<=0){OutError("SPINBOX/SCALE: STEP musi byc dodatni");return 0;}
        if(s->signed_value<s->signed_min||(!s->scale_wrap&&s->signed_value>s->signed_max)||
           (s->kind==NYOTA_UI_CTRL_SCALE&&s->scale_wrap&&s->signed_value>=s->signed_max)){OutError("SPINBOX/SCALE: VALUE poza zakresem");return 0;}
        if(s->kind==NYOTA_UI_CTRL_SCALE&&s->scale_shape!=NYOTA_UI_SCALE_LINE&&s->end_angle==s->start_angle){OutError("SCALE: STARTANGLE i ENDANGLE nie moga byc rowne");return 0;}
    }
    if(s->kind==NYOTA_UI_CTRL_LISTVIEW||s->kind==NYOTA_UI_CTRL_TREEVIEW){
        uint32_t n=UiItemsCount(s->items);
        if(n&&s->selected>=n){OutError("LISTVIEW/TREEVIEW: SELECTED poza ITEMS");return 0;}
    }
    if(s->kind==NYOTA_UI_CTRL_SPLITTER){
        if(s->range_max<=s->range_min||s->range_value<s->range_min||s->range_value>s->range_max){OutError("SPLITTER: nieprawidlowy MIN/MAX/VALUE");return 0;}
    }
    if(s->kind==NYOTA_UI_CTRL_CLOCK){
        uint32_t i;
        if(!s->clock_needles||s->clock_needles>NYOTA_UI_CLOCK_MAX_NEEDLES){OutError("CLOCK: NEEDLES poza zakresem 1..8");return 0;}
        if(s->clock_value_count>s->clock_needles||s->clock_color_count>s->clock_needles||s->clock_shape_count>s->clock_needles||s->clock_width_count>s->clock_needles||s->clock_len_count>s->clock_needles||s->clock_blur_count>s->clock_needles||s->clock_min_count>s->clock_needles||s->clock_max_count>s->clock_needles){OutError("CLOCK: lista parametrow wskazowek dluzsza niz NEEDLES");return 0;}
        if(s->clock_shape==NYOTA_UI_CLOCK_ARC&&s->end_angle==s->start_angle){OutError("CLOCK: STARTANGLE i ENDANGLE nie moga byc rowne");return 0;}
        for(i=0;i<s->clock_needles;i++){
            if(s->clock_needle_max[i]<=s->clock_needle_min[i]){OutError("CLOCK: NEEDLEMAX musi byc wieksze od NEEDLEMIN");return 0;}
            if(i<s->clock_value_count&&(s->clock_values[i]<s->clock_needle_min[i]||s->clock_values[i]>s->clock_needle_max[i])){OutError("CLOCK: wartosc wskazowki poza zakresem");return 0;}
        }
    }
    return 1;
}

static int UiParseNamedProperty(NyotaUiControl *ctl,const char *arg){char p[64];const char*s=NTrim(arg),*eq;uint32_t n=ParseIdent(s,p,sizeof(p));if(!p[0]){OutError("NyotaUI: oczekiwano WLASCIWOSC=wartosc");return 0;}eq=NTrim(s+n);if(*eq!='='){OutError("NyotaUI: oczekiwano WLASCIWOSC=wartosc");return 0;}return UiApplyControlProperty(ctl,p,NTrim(eq+1));}
static int UiApplyControlConfig(uint32_t ln,const char *raw,NyotaUiControl *ctl){uint32_t ind=NIndent(raw),endb=SkipBlock(ln+1,ind),i;for(i=ln+1;i<endb;i++){const char*cl=NTrim(g_lines[i]);if(!*cl||*cl=='#')continue;g_cur_line=i;if(NIndent(g_lines[i])!=ind+NYOTA_INDENT){OutError("NyotaUI CONFIG: wymagane dokladnie 4 spacje");g_cur_line=endb;return 0;}if(!UiParseNamedProperty(ctl,cl)){g_cur_line=endb;return 0;}}if(!UiValidateControlSpec(&ctl->spec)){g_cur_line=endb;return 0;}if(HostUiControlUpdate(ctl->host_handle,&ctl->spec)!=0){OutError("NyotaUI CONFIG: host odrzucil ustawienie");g_cur_line=endb;return 0;}g_cur_line=endb;return 1;}

static int UiControlKindFromLine(const char *l,uint8_t*k,uint32_t*n){
    if(PeekWord(l,"BUTTON")){*k=NYOTA_UI_CTRL_BUTTON;*n=6;return 1;} if(PeekWord(l,"LABEL")){*k=NYOTA_UI_CTRL_LABEL;*n=5;return 1;}
    if(PeekWord(l,"PANEL")){*k=NYOTA_UI_CTRL_PANEL;*n=5;return 1;} if(PeekWord(l,"DAREA")){*k=NYOTA_UI_CTRL_DAREA;*n=5;return 1;}
    if(PeekWord(l,"CBOX")){*k=NYOTA_UI_CTRL_CBOX;*n=4;return 1;} if(PeekWord(l,"COMBO")){*k=NYOTA_UI_CTRL_COMBO;*n=5;return 1;}
    if(PeekWord(l,"SEP")){*k=NYOTA_UI_CTRL_SEP;*n=3;return 1;} if(PeekWord(l,"RADIO")){*k=NYOTA_UI_CTRL_RADIO;*n=5;return 1;}
    if(PeekWord(l,"TABS")){*k=NYOTA_UI_CTRL_TABS;*n=4;return 1;} if(PeekWord(l,"TAB")){*k=NYOTA_UI_CTRL_TAB;*n=3;return 1;}
    if(PeekWord(l,"TAREA")){*k=NYOTA_UI_CTRL_TAREA;*n=5;return 1;}
    if(PeekWord(l,"SBAR")){*k=NYOTA_UI_CTRL_SBAR;*n=4;return 1;} if(PeekWord(l,"PBAR")){*k=NYOTA_UI_CTRL_PBAR;*n=4;return 1;}
    if(PeekWord(l,"EQBOX")){*k=NYOTA_UI_CTRL_EQBOX;*n=5;return 1;} if(PeekWord(l,"SLIDER")){*k=NYOTA_UI_CTRL_SLIDER;*n=6;return 1;}
    if(PeekWord(l,"STATBAR")){*k=NYOTA_UI_CTRL_STATBAR;*n=7;return 1;} if(PeekWord(l,"TOOLBAR")){*k=NYOTA_UI_CTRL_TOOLBAR;*n=7;return 1;}
    if(PeekWord(l,"TBOX")){*k=NYOTA_UI_CTRL_TBOX;*n=4;return 1;} if(PeekWord(l,"SPINBOX")){*k=NYOTA_UI_CTRL_SPINBOX;*n=7;return 1;}
    if(PeekWord(l,"LISTVIEW")){*k=NYOTA_UI_CTRL_LISTVIEW;*n=8;return 1;} if(PeekWord(l,"TREEVIEW")){*k=NYOTA_UI_CTRL_TREEVIEW;*n=8;return 1;}
    if(PeekWord(l,"SPLITTER")){*k=NYOTA_UI_CTRL_SPLITTER;*n=8;return 1;} if(PeekWord(l,"SCALE")){*k=NYOTA_UI_CTRL_SCALE;*n=5;return 1;}
    if(PeekWord(l,"CLOCK")){*k=NYOTA_UI_CTRL_CLOCK;*n=5;return 1;} return 0;
}
static int UiResolveParent(const char *arg,uint8_t kind,NyotaWindow **pw,NyotaUiControl **pc,char *name,uint32_t cap){
    uint32_t pn=ParseIdent(NTrim(arg),name,cap);*pw=0;*pc=0;
    if(!name[0]||*NTrim(NTrim(arg)+pn)||NStrEq(name,"ROOT")){OutError("NyotaUI: nieprawidlowy rodzic");return 0;}
    *pw=FindWindow(name);
    if(kind==NYOTA_UI_CTRL_TAB){
        if(*pw){OutError("TAB: rodzic musi byc TABS");return 0;}
        *pc=FindUiControl(name);if(!*pc||(*pc)->spec.kind!=NYOTA_UI_CTRL_TABS){OutError("TAB: rodzic musi byc TABS");return 0;}return 1;
    }
    if(*pw)return 1;
    *pc=FindUiControl(name);
    if(!*pc||((*pc)->spec.kind!=NYOTA_UI_CTRL_PANEL&&(*pc)->spec.kind!=NYOTA_UI_CTRL_TAB&&(*pc)->spec.kind!=NYOTA_UI_CTRL_STATBAR&&(*pc)->spec.kind!=NYOTA_UI_CTRL_TOOLBAR)){OutError("NyotaUI: rodzic musi byc WIN, PANEL, TAB, STATBAR albo TOOLBAR");return 0;}
    return 1;
}
static void UiInheritWindowStyle(NyotaUiControl *c){
    uint32_t i;for(i=0;i<g_window_count;i++)if(g_windows[i].host_handle==c->window_handle){c->spec.shadow=g_windows[i].shadow;c->spec.shadow_color=g_windows[i].shadow_color;c->spec.shadow_depth=g_windows[i].shadow_depth;break;}
}

/* Returns 1 when consumed as NyotaUI. Returns 0 only for legacy GRAPH BUTTON. */
static int UiExecControl(uint32_t ln,const char *raw,const char *line){
    uint8_t kind;uint32_t kwlen;const char*p;char name[64];uint32_t nn;NyotaUiControl*existing;
    if(!UiControlKindFromLine(line,&kind,&kwlen))return 0;p=NTrim(line+kwlen);nn=ParseIdent(p,name,sizeof(name));if(!name[0]){OutError("NyotaUI: brak nazwy kontrolki");return 1;}p=NTrim(p+nn);
    if(NStrEq(p,".CONFIG:")){existing=FindUiControl(name);if(!existing||existing->spec.kind!=kind){g_cur_line=SkipBlock(ln+1,NIndent(raw));OutError("NyotaUI CONFIG: nieznana kontrolka");return 1;}UiApplyControlConfig(ln,raw,existing);return 1;}
    if(*p!=','){OutError("NyotaUI: po nazwie wymagany przecinek");return 1;}
    {
        char args[64][MAX_STR_LEN],parent_name[64];int n=SplitFunctionArgs(NTrim(p+1),args,64),i,first_prop=0;NyotaWindow*pw=0;NyotaUiControl*pc=0,temp;int32_t wi=0,hi=0,x=0,y=0,handle;uint8_t pos=NYOTA_UI_POS_XY;
        if(kind==NYOTA_UI_CTRL_BUTTON&&n==13){uint32_t pn=ParseIdent(NTrim(args[0]),parent_name,sizeof(parent_name));if(!parent_name[0]||(!FindWindow(parent_name)&&!FindUiControl(parent_name)))return 0;}
        if(FindUiControl(name)||FindWindow(name)){OutError("NyotaUI: nazwa juz istnieje");return 1;}if(g_ui_control_count>=MAX_UI_CONTROLS){OutError("NyotaUI: przekroczono limit kontrolek");return 1;}if(!g_host||!g_host->ui_control_create||!g_host->ui_control_update){OutError("NyotaUI: host nie obsluguje kontrolek");return 1;}
        if(n<2||!UiResolveParent(args[0],kind,&pw,&pc,parent_name,sizeof(parent_name)))return 1;
        memset(&temp,0,sizeof(temp));NStrCopy(temp.name,name,sizeof(temp.name));NStrCopy(temp.parent_name,parent_name,sizeof(temp.parent_name));UiControlDefaults(&temp.spec,kind);
        if(pw){temp.window_handle=pw->host_handle;temp.parent_control_handle=0;}else{temp.window_handle=pc->window_handle;temp.parent_control_handle=pc->host_handle;}
        if(kind==NYOTA_UI_CTRL_TAB){
            NyotaVal title;uint32_t idx=0,j;
            if(n<2){OutError("TAB nazwa, TABS, \"tytul\"");return 1;} title=Eval(args[1]);if(title.type!=TYPE_STR||!title.s[0]){OutError("TAB: tytul wymaga niepustego STRING");return 1;}NStrCopy(temp.spec.text,title.s,sizeof(temp.spec.text));
            temp.spec.w=pc->spec.w;temp.spec.h=pc->spec.h;temp.spec.x=0;temp.spec.y=0;
            for(j=0;j<g_ui_control_count;j++)if(g_ui_controls[j].parent_control_handle==pc->host_handle&&g_ui_controls[j].spec.kind==NYOTA_UI_CTRL_TAB)idx++;
            temp.spec.tab_index=idx;first_prop=2;
        } else if(kind==NYOTA_UI_CTRL_CBOX||kind==NYOTA_UI_CTRL_RADIO){
            int32_t size=1;
            if(NStrEq(NTrim(args[1]),"AUTO"))pos=NYOTA_UI_POS_AUTO;else if(!UiEvalPair(args[1],&x,&y,0,"CBOX/RADIO: pozycja"))return 1;first_prop=2;
            if(n>2){NyotaVal sv=Eval(args[2]);if(sv.type==TYPE_INT){if(sv.i<1||sv.i>64){OutError("CBOX/RADIO: size wymaga INTEGER 1..64");return 1;}size=sv.i;first_prop=3;}}
            temp.spec.logical_size=(uint32_t)size;temp.spec.w=temp.spec.h=NYOTA_UI_CHECK_BASE*(uint32_t)size;temp.spec.x=x;temp.spec.y=y;temp.spec.position_mode=pos;
        } else if(kind==NYOTA_UI_CTRL_SEP){
            NyotaVal lv;int32_t len;
            if(n<3){OutError("SEP nazwa, rodzic, [x,y]/AUTO, dlugosc [, HORIZONTAL/VERTICAL]");return 1;}if(NStrEq(NTrim(args[1]),"AUTO"))pos=NYOTA_UI_POS_AUTO;else if(!UiEvalPair(args[1],&x,&y,0,"SEP: pozycja"))return 1;lv=Eval(args[2]);if(lv.type!=TYPE_INT||lv.i<=0){OutError("SEP: dlugosc wymaga dodatniego INTEGER");return 1;}len=lv.i;first_prop=3;
            if(n>3&&(NStrEq(NTrim(args[3]),"HORIZONTAL")||NStrEq(NTrim(args[3]),"VERTICAL"))){temp.spec.orientation=NStrEq(NTrim(args[3]),"VERTICAL")?NYOTA_UI_SEP_VERTICAL:NYOTA_UI_SEP_HORIZONTAL;first_prop=4;}
            temp.spec.x=x;temp.spec.y=y;temp.spec.position_mode=pos;if(temp.spec.orientation==NYOTA_UI_SEP_VERTICAL){temp.spec.w=1;temp.spec.h=(uint32_t)len;}else{temp.spec.w=(uint32_t)len;temp.spec.h=1;}
        } else {
            if(n<3){OutError("NyotaUI: wymagane rodzic, [w,h], [x,y]/CENTER/AUTO");return 1;}if(!UiEvalPair(args[1],&wi,&hi,1,"NyotaUI: rozmiar"))return 1;
            if(NStrEq(NTrim(args[2]),"CENTER"))pos=NYOTA_UI_POS_CENTER;else if(NStrEq(NTrim(args[2]),"AUTO"))pos=NYOTA_UI_POS_AUTO;else{if(!UiEvalPair(args[2],&x,&y,0,"NyotaUI: pozycja"))return 1;}
            temp.spec.w=(uint32_t)wi;temp.spec.h=(uint32_t)hi;temp.spec.x=x;temp.spec.y=y;temp.spec.position_mode=pos;first_prop=3;
        }
        UiInheritWindowStyle(&temp);
        for(i=first_prop;i<n;i++)if(!UiParseNamedProperty(&temp,args[i]))return 1;
        if(!UiValidateControlSpec(&temp.spec))return 1;
        handle=HostUiControlCreate(temp.name,temp.window_handle,temp.parent_control_handle,&temp.spec);if(handle<=0){OutError("NyotaUI: host nie utworzyl kontrolki");return 1;}temp.host_handle=handle;g_ui_controls[g_ui_control_count++]=temp;
    }
    return 1;
}

// ============================================================
// POMOCNICZE: RYSOWANIE LINII (Bresenham)
// ============================================================
static void DrawLine(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    int dx = x1 - x0; if (dx < 0) dx = -dx;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 - y0; if (dy < 0) dy = -dy;
    int sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    while (1) {
        if (x0 >= 0 && y0 >= 0) HostRect((uint32_t)x0, (uint32_t)y0, 1, 1, r, g, b);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}


// ============================================================
// TABLE — nazwana kontrolka prezentacji danych
// ============================================================
static NyotaTable *FindTable(const char *name) {
    uint32_t i;
    for (i = 0; i < g_table_count; i++)
        if (NStrEq(g_tables[i].name, name)) return &g_tables[i];
    return 0;
}

static NyotaTable *GetOrCreateTable(const char *name) {
    NyotaTable *t = FindTable(name);
    if (t) return t;
    if (g_table_count >= MAX_TABLES) {
        OutError("Za duzo kontrolek TABLE");
        return 0;
    }
    t = &g_tables[g_table_count++];
    NStrCopy(t->name, name, sizeof(t->name));
    t->source_name[0] = '\0';
    return t;
}

static int TableEvalInt(const char *expr, int32_t *out) {
    NyotaVal v = Eval(expr);
    if (v.type != TYPE_INT) {
        OutError("TABLE: parametr liczbowy wymaga INTEGER");
        return 0;
    }
    *out = v.i;
    return 1;
}

static int TableValidateSource(const NyotaTable *t, const NyotaVal *src) {
    uint32_t i;
    if (!src) return 1;
    if (src->type == TYPE_MARK) {
        if (t->columns != (uint32_t)src->i + 1U) {
            OutError("TABLE: MARK wymaga kolumn = klucz + kolumny wartosci");
            return 0;
        }
        return 1;
    }
    if (src->type != TYPE_LIST && src->type != TYPE_TUPLE) {
        OutError("TABLE: zrodlo musi byc LIST, TUPLE albo MARK");
        return 0;
    }
    if (src->list_len == 0) return 1;
    if (t->columns == 1) {
        int nested = src->list_items[0].type == TYPE_LIST || src->list_items[0].type == TYPE_TUPLE;
        if (!nested) return 1;
    }
    for (i = 0; i < src->list_len; i++) {
        const NyotaVal *row = &src->list_items[i];
        if (row->type != TYPE_LIST && row->type != TYPE_TUPLE) {
            OutError("TABLE: wielokolumnowe LIST/TUPLE wymaga wierszy LIST/TUPLE");
            return 0;
        }
        if (row->list_len != t->columns) {
            OutError("TABLE: wiersz ma zla liczbe kolumn");
            return 0;
        }
    }
    return 1;
}

static uint32_t TableRowCount(const NyotaTable *t, const NyotaVal *src) {
    (void)t;
    if (!src) return 0;
    return src->list_len;
}

static const NyotaVal *TableCell(const NyotaTable *t, const NyotaVal *src,
                                 uint32_t row, uint32_t col) {
    if (!src || row >= src->list_len || col >= t->columns) return 0;
    if (src->type == TYPE_MARK) {
        const NyotaVal *r = &src->list_items[row];
        if (r->type != TYPE_LIST || col >= r->list_len) return 0;
        return &r->list_items[col];
    }
    if (src->type == TYPE_LIST || src->type == TYPE_TUPLE) {
        const NyotaVal *r = &src->list_items[row];
        if (t->columns == 1 && r->type != TYPE_LIST && r->type != TYPE_TUPLE)
            return r;
        if ((r->type == TYPE_LIST || r->type == TYPE_TUPLE) && col < r->list_len)
            return &r->list_items[col];
    }
    return 0;
}

static void TableClipText(const char *src, char *dst, uint32_t dst_size,
                          int32_t cell_w, uint32_t scale) {
    uint32_t max_chars, i = 0;
    if (!dst_size) return;
    if (scale < 1) scale = 1;
    if (cell_w <= 8) { dst[0] = '\0'; return; }
    max_chars = (uint32_t)(cell_w - 8) / (8U * scale);
    if (max_chars + 1 > dst_size) max_chars = dst_size - 1;
    while (src[i] && i < max_chars) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int RenderTable(NyotaTable *t) {
    NyotaVar *sv = 0;
    NyotaVal *src = 0;
    uint32_t scale, row_h, rows, visible, rix, cix;
    int32_t xx;
    if (!g_is_graphics) {
        OutError("TABLE wymaga GRAPH");
        return 0;
    }
    if (t->source_name[0]) {
        sv = FindVar(t->source_name);
        if (!sv) {
            OutError("TABLE: zrodlo danych nie istnieje");
            return 0;
        }
        src = &sv->val;
        if (!TableValidateSource(t, src)) return 0;
    }
    scale = (t->font_size + 7U) / 8U;
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    row_h = 8U * scale + 6U;
    rows = TableRowCount(t, src);
    visible = (t->h > 2) ? (uint32_t)(t->h - 2) / row_h : 0;
    if (visible > rows) visible = rows;

    // Ramka i pionowe podzialy. Kolor siatki jest neutralny; kolor tekstu jest parametrem TABLE.
    DrawLine(t->x, t->y, t->x + t->w - 1, t->y, 96, 96, 96);
    DrawLine(t->x, t->y + t->h - 1, t->x + t->w - 1, t->y + t->h - 1, 96, 96, 96);
    DrawLine(t->x, t->y, t->x, t->y + t->h - 1, 96, 96, 96);
    DrawLine(t->x + t->w - 1, t->y, t->x + t->w - 1, t->y + t->h - 1, 96, 96, 96);
    xx = t->x;
    for (cix = 0; cix + 1 < t->columns; cix++) {
        xx += t->widths[cix];
        DrawLine(xx, t->y, xx, t->y + t->h - 1, 96, 96, 96);
    }

    for (rix = 0; rix < visible; rix++) {
        int32_t yy = t->y + (int32_t)(rix * row_h);
        xx = t->x;
        for (cix = 0; cix < t->columns; cix++) {
            const NyotaVal *cell = TableCell(t, src, rix, cix);
            if (cell) {
                char raw[MAX_STR_LEN], clipped[MAX_STR_LEN];
                ValToStr(cell, raw, sizeof(raw));
                TableClipText(raw, clipped, sizeof(clipped), t->widths[cix], scale);
                HostText((uint32_t)(xx + 4), (uint32_t)(yy + 3), clipped,
                         t->r, t->g, t->b, scale);
            }
            xx += t->widths[cix];
        }
        if (rix + 1 < visible) {
            int32_t hy = t->y + (int32_t)((rix + 1) * row_h);
            if (hy < t->y + t->h - 1)
                DrawLine(t->x, hy, t->x + t->w - 1, hy, 96, 96, 96);
        }
    }
    return 1;
}

// ============================================================
// BUTTON — nazwana kontrolka GUI
// ============================================================
static NyotaButton *FindButton(const char *name) {
    uint32_t i;
    for (i = 0; i < g_button_count; i++)
        if (NStrEq(g_buttons[i].name, name)) return &g_buttons[i];
    return 0;
}

static NyotaButton *GetOrCreateButton(const char *name) {
    NyotaButton *b = FindButton(name);
    if (b) return b;
    if (g_button_count >= MAX_BUTTONS) {
        OutError("Za duzo kontrolek BUTTON");
        return 0;
    }
    b = &g_buttons[g_button_count++];
    NStrCopy(b->name, name, sizeof(b->name));
    b->prev_down = 0;
    return b;
}

static int ButtonEvalInt(const char *expr, int32_t *out) {
    NyotaVal v = Eval(expr);
    if (v.type != TYPE_INT) {
        OutError("BUTTON: parametr liczbowy wymaga INTEGER");
        return 0;
    }
    *out = v.i;
    return 1;
}

static void RenderButton(NyotaButton *b) {
    uint32_t scale, text_w, text_h;
    int32_t tx, ty;
    char clipped[MAX_STR_LEN];
    if (!b || !g_is_graphics) return;

    HostRect((uint32_t)b->x, (uint32_t)b->y, (uint32_t)b->w, (uint32_t)b->h,
             b->bg_r, b->bg_g, b->bg_b);
    DrawLine(b->x, b->y, b->x + b->w - 1, b->y, 190, 190, 190);
    DrawLine(b->x, b->y, b->x, b->y + b->h - 1, 190, 190, 190);
    DrawLine(b->x, b->y + b->h - 1, b->x + b->w - 1, b->y + b->h - 1, 55, 55, 55);
    DrawLine(b->x + b->w - 1, b->y, b->x + b->w - 1, b->y + b->h - 1, 55, 55, 55);

    scale = (b->font_size + 7U) / 8U;
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    TableClipText(b->text, clipped, sizeof(clipped), b->w - 8, scale);
    text_w = NStrLen(clipped) * 8U * scale;
    text_h = 8U * scale;
    tx = b->x + (b->w - (int32_t)text_w) / 2;
    ty = b->y + (b->h - (int32_t)text_h) / 2;
    if (tx < b->x + 2) tx = b->x + 2;
    if (ty < b->y + 2) ty = b->y + 2;
    HostText((uint32_t)tx, (uint32_t)ty, clipped,
             b->text_r, b->text_g, b->text_b, scale);
}

static int ButtonPollClicked(NyotaButton *b) {
    int32_t mx = 0, my = 0;
    uint8_t buttons, down;
    int inside, clicked;
    if (!b) return 0;
    if (!g_host || !g_host->pointer_state) {
        OutError("BUTTON_CLICKED: host nie obsluguje wskaznika");
        return -1;
    }
    buttons = HostPointerState(&mx, &my);
    down = buttons & 1U;
    inside = mx >= b->x && my >= b->y && mx < b->x + b->w && my < b->y + b->h;
    clicked = down && !b->prev_down && inside;
    b->prev_down = down;
    return clicked ? 1 : 0;
}


// ============================================================
// SPRITE — nazwany obiekt graficzny
// ============================================================
static NyotaSprite *FindSprite(const char *name) {
    uint32_t i;
    for (i = 0; i < g_sprite_count; i++)
        if (NStrEq(g_sprites[i].name, name)) return &g_sprites[i];
    return 0;
}

static int32_t FindSpriteIndex(const char *name) {
    uint32_t i;
    for (i = 0; i < g_sprite_count; i++)
        if (NStrEq(g_sprites[i].name, name)) return (int32_t)i;
    return -1;
}

static NyotaSprite *GetOrCreateSprite(const char *name) {
    NyotaSprite *s = FindSprite(name);
    if (s) return s;
    if (g_sprite_count >= MAX_SPRITES) {
        OutError("Za duzo obiektow SPRITE");
        return 0;
    }
    s = &g_sprites[g_sprite_count++];
    s->name[0] = '\0';
    s->source[0] = '\0';
    s->x = s->y = 0;
    s->w = s->h = 1;
    s->host_handle = -1;
    s->visible = 1;
    s->frame_count = 1;
    s->frame = 0;
    s->frame_ms = 100;
    s->playing = 0;
    s->last_frame_tick = 0;
    NStrCopy(s->name, name, sizeof(s->name));
    return s;
}

static int SpriteEvalInt(const char *expr, int32_t *out) {
    NyotaVal v = Eval(expr);
    if (v.type != TYPE_INT) {
        OutError("SPRITE: parametr liczbowy wymaga INTEGER");
        return 0;
    }
    *out = v.i;
    return 1;
}

static int SpriteArgName(const char *arg, char *out, uint32_t out_size) {
    const char *p = NTrim(arg);
    if (*p == '"') {
        NyotaVal v = Eval(p);
        if (v.type != TYPE_STR || !v.s[0]) return 0;
        NStrCopy(out, v.s, out_size);
        return 1;
    }
    {
        uint32_t n = ParseIdent(p, out, out_size);
        if (!out[0] || *NTrim(p + n)) return 0;
    }
    return 1;
}

static int SpriteHit(const NyotaSprite *a, const NyotaSprite *b) {
    int64_t ax2, ay2, bx2, by2;
    if (!a || !b) return 0;
    ax2 = (int64_t)a->x + a->w;
    ay2 = (int64_t)a->y + a->h;
    bx2 = (int64_t)b->x + b->w;
    by2 = (int64_t)b->y + b->h;
    return (int64_t)a->x < bx2 && ax2 > (int64_t)b->x &&
           (int64_t)a->y < by2 && ay2 > (int64_t)b->y;
}

static void SpriteUpdateAnimation(NyotaSprite *s) {
    uint64_t now, frame_ticks, elapsed, steps;
    if (!s || !s->playing || s->frame_count <= 1 || s->frame_ms == 0) return;
    now = HostTicks();
    frame_ticks = ((uint64_t)s->frame_ms + 9ULL) / 10ULL;
    if (frame_ticks < 1) frame_ticks = 1;
    if (s->last_frame_tick == 0) {
        s->last_frame_tick = now;
        return;
    }
    elapsed = now - s->last_frame_tick;
    steps = elapsed / frame_ticks;
    if (steps == 0) return;
    s->frame = (uint16_t)(((uint64_t)s->frame + steps) % s->frame_count);
    s->last_frame_tick += steps * frame_ticks;
}

static int RenderSprite(NyotaSprite *s) {
    if (!s || !s->visible) return 1;
    if (!g_is_graphics) {
        OutError("SPRITE_DRAW wymaga GRAPH");
        return 0;
    }
    if (s->host_handle < 0) {
        OutError("SPRITE_DRAW: zasob nie jest zaladowany");
        return 0;
    }
    SpriteUpdateAnimation(s);
    if (s->frame_count > 1) {
        if (!HostSpriteDrawFrame(s->host_handle, s->frame, s->frame_count,
                                 s->x, s->y,
                                 (uint32_t)s->w, (uint32_t)s->h)) {
            OutError("SPRITE_DRAW: host nie obsluguje animacji sprite");
            return 0;
        }
    } else if (!HostSpriteDraw(s->host_handle, s->x, s->y,
                               (uint32_t)s->w, (uint32_t)s->h)) {
        OutError("SPRITE_DRAW: host nie obsluguje sprite");
        return 0;
    }
    return 1;
}

static void RemoveSpriteAt(uint32_t index) {
    uint32_t i;
    if (index >= g_sprite_count) return;
    if (g_sprites[index].host_handle >= 0)
        HostSpriteFree(g_sprites[index].host_handle);
    for (i = index + 1; i < g_sprite_count; i++)
        g_sprites[i - 1] = g_sprites[i];
    g_sprite_count--;
}

// ============================================================
// POMIŃ BLOK (skocz za blok wcięty o więcej niż cur_indent)
// ============================================================
static uint32_t SkipBlock(uint32_t from, uint32_t block_indent) {
    uint32_t i = from;
    while (i < g_line_count) {
        const char *line = g_lines[i];
        // Pusta linia — kontynuuj
        if (NStrLen(NTrim(line)) == 0) { i++; continue; }
        // Komentarz — kontynuuj
        if (*NTrim(line) == '#') { i++; continue; }
        uint32_t ind = NIndent(line);
        if (ind <= block_indent) break;
        i++;
    }
    return i;
}

#define NYASM_MAX_IO 8

static int NyasmReg(const char *p) {
    const char *t = NTrim(p);
    if (t[0] == 'R' && t[1] >= '0' && t[1] <= '3' && !t[2]) return t[1] - '0';
    return -1;
}

static int NyasmNameIn(char names[][64], uint32_t count, const char *name) {
    uint32_t i;
    for (i = 0; i < count; i++) if (NStrEq(names[i], name)) return (int)i;
    return -1;
}

static int NyasmParseNames(const char *src, char names[][64], uint32_t *count) {
    const char *p = NTrim(src);
    *count = 0;
    if (!*p) return 1;
    while (*p) {
        char name[64];
        uint32_t n = ParseIdent(p, name, sizeof(name));
        uint32_t i;
        if (!name[0]) return 0;
        p = NTrim(p + n);
        for (i = 0; i < *count; i++) if (NStrEq(names[i], name)) return 0;
        if (*count >= NYASM_MAX_IO) return 0;
        NStrCopy(names[(*count)++], name, 64);
        if (!*p) break;
        if (*p != ',') return 0;
        p = NTrim(p + 1);
    }
    return 1;
}

static int NyasmReadOperand(const char *src, char inputs[][64], uint32_t input_count,
                            int32_t regs[4], int32_t *out) {
    const char *p = NTrim(src);
    int r = NyasmReg(p);
    if (r >= 0) { *out = regs[r]; return 1; }
    if (NStrEq(p, "TRUE")) { *out = 1; return 1; }
    if (NStrEq(p, "FALSE")) { *out = 0; return 1; }
    if (NIsDigit(*p) || (*p == '-' && NIsDigit(p[1]))) {
        NyotaVal v; uint32_t used = 0;
        if (!ParseNumber(p, &v, &used) || v.type != TYPE_INT || *NTrim(p + used)) {
            OutError("NYASM: operand liczbowy musi byc INTEGER");
            return 0;
        }
        *out = v.i;
        return 1;
    }
    if (NIsAlpha(*p)) {
        char name[64];
        uint32_t n = ParseIdent(p, name, sizeof(name));
        NyotaVar *v;
        if (!name[0] || *NTrim(p + n)) { OutError("NYASM: zly operand"); return 0; }
        if (NyasmNameIn(inputs, input_count, name) < 0) {
            OutError("NYASM: odczyt zmiennej spoza INPUT");
            return 0;
        }
        v = FindVar(name);
        if (!v) { OutError("NYASM: INPUT wskazuje nieistniejaca zmienna"); return 0; }
        if (v->val.type != TYPE_INT && v->val.type != TYPE_BOOL) {
            OutError("NYASM: INPUT obsluguje INTEGER/BOOLEAN");
            return 0;
        }
        *out = v->val.i;
        return 1;
    }
    OutError("NYASM: niepoprawny operand");
    return 0;
}

static int NyasmStore(const char *target, int32_t value,
                      char outputs[][64], uint32_t output_count, uint8_t written[]) {
    const char *p = NTrim(target);
    char name[64];
    uint32_t n = ParseIdent(p, name, sizeof(name));
    int oi;
    NyotaVar *v;
    if (!name[0] || *NTrim(p + n)) { OutError("NYASM STORE: zla nazwa OUTPUT"); return 0; }
    oi = NyasmNameIn(outputs, output_count, name);
    if (oi < 0) { OutError("NYASM: zapis zmiennej spoza OUTPUT"); return 0; }
    v = FindVar(name);
    if (!v) {
        v = CreateVar(name);
        if (!v) { OutError("NYASM: za duzo zmiennych"); return 0; }
        ValFromInt(&v->val, value);
    } else {
        if (v->is_const) { OutError("NYASM: OUTPUT nie moze byc CONST"); return 0; }
        if (v->val.type == TYPE_NONE || v->val.type == TYPE_INT) ValFromInt(&v->val, value);
        else if (v->val.type == TYPE_BOOL) ValFromBool(&v->val, value != 0);
        else { OutError("NYASM: OUTPUT obsluguje INTEGER/BOOLEAN"); return 0; }
    }
    written[oi] = 1;
    return 1;
}

static int NyasmCheckedResult(int64_t v, int32_t *out) {
    if (v < INT32_MIN || v > INT32_MAX) {
        OutError("NYASM: przepelnienie INTEGER");
        return 0;
    }
    *out = (int32_t)v;
    return 1;
}

static void ExecNyasmBlock(uint32_t ln, const char *raw, const char *line) {
    char header[MAX_LINE_LEN];
    char inputs[NYASM_MAX_IO][64], outputs[NYASM_MAX_IO][64];
    uint32_t input_count = 0, output_count = 0;
    uint8_t written[NYASM_MAX_IO] = {0};
    int32_t regs[4] = {0,0,0,0};
    uint32_t my_indent = NIndent(raw);
    uint32_t body_start = ln + 1, body_end = SkipBlock(body_start, my_indent);
    const char *p;
    uint32_t len, i;
    int is_long = NStartsWith(line, "NYASM");

    NStrCopy(header, NTrim(line + (is_long ? 5 : 3)), sizeof(header));
    len = NStrLen(header);
    if (!len || header[len - 1] != ':') { OutError("NYASM: naglowek musi konczyc sie :"); g_cur_line = body_end; return; }
    header[--len] = '\0'; NRTrim(header);
    p = NTrim(header);

    if (PeekWord(p, "INPUT")) {
        const char *start, *outp = 0, *q;
        char inbuf[256];
        uint32_t n;
        p = NTrim(p + 5);
        start = p;
        for (q = p; *q; q++) {
            if ((q == p || NIsSpace(q[-1])) && PeekWord(q, "OUTPUT")) { outp = q; break; }
        }
        if (!outp) { OutError("NYASM: po INPUT wymagane OUTPUT"); g_cur_line = body_end; return; }
        n = (uint32_t)(outp - start);
        if (n >= sizeof(inbuf)) { OutError("NYASM: lista INPUT za dluga"); g_cur_line = body_end; return; }
        for (i = 0; i < n; i++) inbuf[i] = start[i];
        inbuf[n] = '\0'; NRTrim(inbuf);
        if (!NyasmParseNames(inbuf, inputs, &input_count)) {
            OutError("NYASM: niepoprawna lista INPUT");
            g_cur_line = body_end; return;
        }
        p = outp;
    }
    if (!PeekWord(p, "OUTPUT")) {
        OutError("NYASM: wymagane OUTPUT");
        g_cur_line = body_end; return;
    }
    p = NTrim(p + 6);
    if (!NyasmParseNames(p, outputs, &output_count) || output_count == 0) {
        OutError("NYASM: niepoprawna lista OUTPUT");
        g_cur_line = body_end; return;
    }

    for (i = body_start; i < body_end; i++) {
        const char *il = NTrim(g_lines[i]);
        char op[32];
        uint32_t on;
        char args[2][MAX_STR_LEN];
        int n;
        int r, rhs;
        int32_t value;
        g_cur_line = i;
        if (!*il || *il == '#') continue;
        if (NIndent(g_lines[i]) != my_indent + NYOTA_INDENT) {
            OutError("NYASM: instrukcja musi miec dokladnie jeden poziom wciecia");
            g_cur_line = body_end; return;
        }
        on = ParseIdent(il, op, sizeof(op));
        if (!op[0]) { OutError("NYASM: brak instrukcji"); g_cur_line = body_end; return; }
        il = NTrim(il + on);

        if (NStrEq(op, "STORE")) {
            n = SplitFunctionArgs(il, args, 2);
            if (n != 2) { OutError("NYASM STORE output, Rn"); g_cur_line = body_end; return; }
            r = NyasmReg(NTrim(args[1]));
            if (r < 0) { OutError("NYASM STORE wymaga rejestru R0..R3"); g_cur_line = body_end; return; }
            if (!NyasmStore(args[0], regs[r], outputs, output_count, written)) { g_cur_line = body_end; return; }
            continue;
        }

        if (NStrEq(op, "MOV") || NStrEq(op, "ADD") || NStrEq(op, "SUB") ||
            NStrEq(op, "MUL") || NStrEq(op, "DIV") || NStrEq(op, "MOD")) {
            n = SplitFunctionArgs(il, args, 2);
            if (n != 2) { OutError("NYASM: instrukcja wymaga Rn, operand"); g_cur_line = body_end; return; }
            r = NyasmReg(NTrim(args[0]));
            if (r < 0) { OutError("NYASM: wymagany rejestr R0..R3"); g_cur_line = body_end; return; }
            if (!NyasmReadOperand(args[1], inputs, input_count, regs, &rhs)) { g_cur_line = body_end; return; }
            if (NStrEq(op, "MOV")) regs[r] = rhs;
            else if (NStrEq(op, "ADD")) {
                if (!NyasmCheckedResult((int64_t)regs[r] + rhs, &value)) { g_cur_line = body_end; return; }
                regs[r] = value;
            } else if (NStrEq(op, "SUB")) {
                if (!NyasmCheckedResult((int64_t)regs[r] - rhs, &value)) { g_cur_line = body_end; return; }
                regs[r] = value;
            } else if (NStrEq(op, "MUL")) {
                if (!NyasmCheckedResult((int64_t)regs[r] * rhs, &value)) { g_cur_line = body_end; return; }
                regs[r] = value;
            } else {
                if (rhs == 0) { OutError(NStrEq(op, "DIV") ? "NYASM DIV: dzielenie przez zero" : "NYASM MOD: dzielenie przez zero"); g_cur_line = body_end; return; }
                if (regs[r] == INT32_MIN && rhs == -1) { OutError("NYASM: przepelnienie INTEGER"); g_cur_line = body_end; return; }
                if (NStrEq(op, "DIV")) regs[r] /= rhs;
                else regs[r] %= rhs;
            }
            continue;
        }

        OutError("NYASM: nieznana instrukcja");
        g_cur_line = body_end; return;
    }

    for (i = 0; i < output_count; i++) {
        if (!written[i]) {
            OutError("NYASM: kazdy OUTPUT musi zostac zapisany przez STORE");
            g_cur_line = body_end; return;
        }
    }
    g_cur_line = body_end;
}

// Wcięcia: tabulator jest błędem, a nowy poziom może powstać wyłącznie
// bezpośrednio po konstrukcji otwierającej blok i dokładnie o +4 spacje.
static int LineOpensIndentedBlock(const char *t) {
    uint32_t n;
    if (!t || !*t) return 0;
    n = NStrLen(t);
    if (n > 0 && t[n - 1] == ':') return 1;
    if (NStrEq(t, "REPEAT") || NStrEq(t, "DO")) return 1;
    return 0;
}

static int ValidateSourceLayout(void) {
    int have_prev = 0;
    uint32_t prev_indent = 0;
    int prev_opens = 0;
    for (uint32_t i = 0; i < g_line_count; i++) {
        g_cur_line = i;
        const char *raw = g_lines[i];
        const char *t;
        uint32_t ind;
        if (NLineHasTab(raw)) {
            OutError("Tabulator jest zabroniony; uzyj 4 spacji na poziom bloku");
            return 0;
        }
        t = NTrim(raw);
        if (!*t || *t == '#') continue;
        ind = NIndent(raw);
        if ((ind % NYOTA_INDENT) != 0) {
            OutError("Wciecie musi byc wielokrotnoscia 4 spacji");
            return 0;
        }
        if (!have_prev) {
            if (ind != 0) {
                OutError("Pierwsza instrukcja nie moze byc wcięta");
                return 0;
            }
        } else if (ind > prev_indent) {
            if (!prev_opens || ind != prev_indent + NYOTA_INDENT) {
                OutError("Nowy poziom bloku musi miec dokladnie +4 spacje po instrukcji otwierajacej blok");
                return 0;
            }
        } else if (prev_opens && ind <= prev_indent) {
            OutError("Instrukcja otwierajaca blok wymaga co najmniej jednej linii z wcieciem +4");
            return 0;
        }
        prev_indent = ind;
        prev_opens = LineOpensIndentedBlock(t);
        have_prev = 1;
    }
    if (have_prev && prev_opens) {
        OutError("Plik konczy sie po instrukcji otwierajacej pusty blok");
        return 0;
    }
    return 1;
}

static NyotaProc *FindProcByName(const char *name) {
    uint32_t i;
    for (i = 0; i < g_proc_count; i++)
        if (NStrEq(g_procs[i].name, name)) return &g_procs[i];
    return 0;
}

static void SchedulerPoll(void) {
    uint64_t now;
    uint32_t i;
    if (g_scheduler_running) return;
    now = HostTicks();
    for (i = 0; i < MAX_EVERY_EVENTS; i++) {
        NyotaEveryEvent *e = &g_every[i];
        if (!e->active || now < e->next_tick) continue;
        /* Jedno wywołanie na punkt schedulera. Jeśli program był zajęty dłużej
         * niż interwał, nie tworzymy lawiny zaległych callbacków. */
        e->next_tick = now + e->interval_ticks;
        g_scheduler_running = 1;
        CallNamed(e->proc_name, "()", 0);
        g_scheduler_running = 0;
    }
}

// ============================================================
// WYKONANIE BLOKU LINII
// ============================================================
static void ExecLines(uint32_t from, uint32_t to, uint32_t block_indent);

static void ExecLine(uint32_t ln, uint32_t block_indent) {
    if (g_exit_flag || g_return_flag || g_continue_flag) return;
    const char *raw = g_lines[ln];
    const char *line = NTrim(raw);
    g_cur_line = ln;

    // Pusta lub komentarz
    if (!*line || *line == '#') return;

    // --- NyotaUI child controls ---
    if (PeekWord(line, "BUTTON") || PeekWord(line, "LABEL") ||
        PeekWord(line, "PANEL") || PeekWord(line, "DAREA") ||
        PeekWord(line, "CBOX") || PeekWord(line, "COMBO") ||
        PeekWord(line, "SEP") || PeekWord(line, "RADIO") ||
        PeekWord(line, "TABS") || PeekWord(line, "TAB") ||
        PeekWord(line, "TAREA") || PeekWord(line, "SBAR") ||
        PeekWord(line, "PBAR") || PeekWord(line, "EQBOX") ||
        PeekWord(line, "SLIDER") || PeekWord(line, "STATBAR") ||
        PeekWord(line, "TOOLBAR") || PeekWord(line, "TBOX") ||
        PeekWord(line, "SPINBOX") || PeekWord(line, "LISTVIEW") ||
        PeekWord(line, "TREEVIEW") || PeekWord(line, "SPLITTER") ||
        PeekWord(line, "SCALE") || PeekWord(line, "CLOCK")) {
        if (UiExecControl(ln, raw, line)) return;
        /* legacy GRAPH BUTTON falls through to its old implementation below */
    }

    // --- NyotaUI WIN / WIN name.CONFIG: ---
    if (PeekWord(line, "WIN")) {
        UiExecWin(ln, raw, line);
        return;
    }

    // --- PRINT ---
    if (NStartsWith(line, "PRINT")) {
        if (g_is_graphics) { OutError("PRINT niedozwolone po wywolaniu GRAPH"); return; }
        const char *arg = NTrim(line + 5);
        if (!*arg) {
            OutNewLine();
            return;
        }
        {
            char args[16][MAX_STR_LEN];
            int n = SplitFunctionArgs(arg, args, 16);
            int i;
            if (n <= 0) {
                NyotaVal v = Eval(arg);
                char buf[MAX_STR_LEN];
                ValToStr(&v, buf, sizeof(buf));
                OutPrint(buf);
            } else {
                for (i = 0; i < n; i++) {
                    NyotaVal v = Eval(args[i]);
                    char buf[MAX_STR_LEN];
                    ValToStr(&v, buf, sizeof(buf));
                    OutPrint(buf);
                    if (i + 1 < n) OutPrint(" ");
                }
            }
            OutNewLine();
        }
        return;
    }

    // --- GOTOXY ---
    if (NStartsWith(line, "GOTOXY")) {
        if (g_is_graphics) { OutError("GOTOXY niedozwolone po wywolaniu GRAPH"); return; }
        int32_t args[2] = {0};
        ParseArgs(NTrim(line + 6), args, 2);
        g_out_x = OUT_MARGIN + args[0] * OUT_FONT_W;
        g_out_y = args[1] * OUT_FONT_H;
        return;
    }

    // --- CLEAR lista  albo  CLEAR r, g, b ---
    if (NStartsWith(line, "CLEAR")) {
        const char *p = NTrim(line + 5);
        char name[64];
        uint32_t nlen = ParseIdent(p, name, sizeof(name));
        if (name[0] && *NTrim(p + nlen) == '\0') {
            NyotaVar *v = FindVar(name);
            if (v && v->val.type == TYPE_TUPLE) {
                OutError("TUPLE jest niemutowalne");
                return;
            }
            if (v && (v->val.type == TYPE_LIST || v->val.type == TYPE_MARK)) {
                if (v->is_const) { OutError("Nie mozna zmienic stalej kolekcji"); return; }
                v->val.list_len = 0;
                return;
            }
        }
        {
            int32_t args[3] = {0};
            ParseArgs(p, args, 3);
            HostClear((uint8_t)args[0], (uint8_t)args[1], (uint8_t)args[2]);
            g_out_x = OUT_MARGIN; g_out_y = OUT_MARGIN;
        }
        return;
    }

    // --- BOX x, y, w, h, r, g, b, [f] ---
    if (NStartsWith(line, "BOX")) {
        int32_t args[8] = {0};
        int pc = ParseArgs(NTrim(line + 3), args, 8);
        uint8_t r = (uint8_t)args[4], g = (uint8_t)args[5], b = (uint8_t)args[6];
        uint8_t f = (pc >= 8) ? (uint8_t)args[7] : 0;
        if (f) {
            HostRect((uint32_t)args[0], (uint32_t)args[1], (uint32_t)args[2], (uint32_t)args[3], r, g, b);
        } else {
            DrawLine(args[0], args[1], args[0]+args[2]-1, args[1], r, g, b);
            DrawLine(args[0]+args[2]-1, args[1], args[0]+args[2]-1, args[1]+args[3]-1, r, g, b);
            DrawLine(args[0]+args[2]-1, args[1]+args[3]-1, args[0], args[1]+args[3]-1, r, g, b);
            DrawLine(args[0], args[1]+args[3]-1, args[0], args[1], r, g, b);
        }
        return;
    }

    // --- LINE x1, y1, x2, y2, r, g, b ---
    if (NStartsWith(line, "LINE")) {
        int32_t args[7] = {0};
        ParseArgs(NTrim(line + 4), args, 7);
        DrawLine(args[0], args[1], args[2], args[3], (uint8_t)args[4], (uint8_t)args[5], (uint8_t)args[6]);
        return;
    }

    // --- CIRCLE x, y, r, R, G, B, [f] ---
    if (NStartsWith(line, "CIRCLE")) {
        int32_t args[7] = {0};
        int pc = ParseArgs(NTrim(line + 6), args, 7);
        int xc = args[0], yc = args[1], rad = args[2];
        uint8_t r = (uint8_t)args[3], g = (uint8_t)args[4], b = (uint8_t)args[5];
        uint8_t f = (pc >= 7) ? (uint8_t)args[6] : 0;
        if (f) {
            for (int y = -rad; y <= rad; y++) {
                for (int x = -rad; x <= rad; x++) {
                    if (x*x + y*y <= rad*rad) {
                        if (xc+x >= 0 && yc+y >= 0) HostRect((uint32_t)(xc+x), (uint32_t)(yc+y), 1, 1, r, g, b);
                    }
                }
            }
        } else {
            int cx = 0, cy = rad, d = 3 - 2 * rad;
            while (cy >= cx) {
                if (xc+cx>=0 && yc+cy>=0) HostRect(xc+cx, yc+cy, 1, 1, r, g, b);
                if (xc-cx>=0 && yc+cy>=0) HostRect(xc-cx, yc+cy, 1, 1, r, g, b);
                if (xc+cx>=0 && yc-cy>=0) HostRect(xc+cx, yc-cy, 1, 1, r, g, b);
                if (xc-cx>=0 && yc-cy>=0) HostRect(xc-cx, yc-cy, 1, 1, r, g, b);
                if (xc+cy>=0 && yc+cx>=0) HostRect(xc+cy, yc+cx, 1, 1, r, g, b);
                if (xc-cy>=0 && yc+cx>=0) HostRect(xc-cy, yc+cx, 1, 1, r, g, b);
                if (xc+cy>=0 && yc-cx>=0) HostRect(xc+cy, yc-cx, 1, 1, r, g, b);
                if (xc-cy>=0 && yc-cx>=0) HostRect(xc-cy, yc-cx, 1, 1, r, g, b);
                cx++;
                if (d > 0) { cy--; d += 4 * (cx - cy) + 10; }
                else { d += 4 * cx + 6; }
            }
        }
        return;
    }

    // --- ELLIPSE x, y, rx, ry, r, g, b, [f] ---
    if (NStartsWith(line, "ELLIPSE")) {
        int32_t args[8] = {0};
        int pc = ParseArgs(NTrim(line + 7), args, 8);
        int xc = args[0], yc = args[1], rx = args[2], ry = args[3];
        uint8_t cr = (uint8_t)args[4], cg = (uint8_t)args[5], cb = (uint8_t)args[6];
        uint8_t f = (pc >= 8) ? (uint8_t)args[7] : 0;
        if (rx > 0 && ry > 0) {
            for (int y = -ry; y <= ry; y++) {
                for (int x = -rx; x <= rx; x++) {
                    if (x*x*ry*ry + y*y*rx*rx <= rx*rx*ry*ry) {
                        if (f || (x*x*ry*ry + y*y*rx*rx >= rx*rx*ry*ry - (rx*ry*2))) {
                            if (xc+x >= 0 && yc+y >= 0)
                                HostRect((uint32_t)(xc+x), (uint32_t)(yc+y), 1, 1, cr, cg, cb);
                        }
                    }
                }
            }
        }
        return;
    }

    // --- TRIANGLE x1, y1, x2, y2, x3, y3, r, g, b, [f] ---
    if (NStartsWith(line, "TRIANGLE")) {
        int32_t args[10] = {0};
        int pc = ParseArgs(NTrim(line + 8), args, 10);
        uint8_t r = (uint8_t)args[6], g = (uint8_t)args[7], b = (uint8_t)args[8];
        uint8_t f = (pc >= 10) ? (uint8_t)args[9] : 0;
        if (f) {
            int min_x = args[0]; if(args[2] < min_x) min_x = args[2]; if(args[4] < min_x) min_x = args[4];
            int max_x = args[0]; if(args[2] > max_x) max_x = args[2]; if(args[4] > max_x) max_x = args[4];
            int min_y = args[1]; if(args[3] < min_y) min_y = args[3]; if(args[5] < min_y) min_y = args[5];
            int max_y = args[1]; if(args[3] > max_y) max_y = args[3]; if(args[5] > max_y) max_y = args[5];
            for (int y = min_y; y <= max_y; y++) {
                for (int x = min_x; x <= max_x; x++) {
                    int w0 = (args[2]-args[0])*(y-args[1]) - (args[3]-args[1])*(x-args[0]);
                    int w1 = (args[4]-args[2])*(y-args[3]) - (args[5]-args[3])*(x-args[2]);
                    int w2 = (args[0]-args[4])*(y-args[5]) - (args[1]-args[5])*(x-args[4]);
                    if ((w0>=0 && w1>=0 && w2>=0) || (w0<=0 && w1<=0 && w2<=0)) {
                        if (x>=0 && y>=0) HostRect((uint32_t)x, (uint32_t)y, 1, 1, r, g, b);
                    }
                }
            }
        } else {
            DrawLine(args[0], args[1], args[2], args[3], r, g, b);
            DrawLine(args[2], args[3], args[4], args[5], r, g, b);
            DrawLine(args[4], args[5], args[0], args[1], r, g, b);
        }
        return;
    }

    // --- QUAD x1, y1, x2, y2, x3, y3, x4, y4, r, g, b, [f] ---
    if (NStartsWith(line, "QUAD")) {
        int32_t args[12] = {0};
        int pc = ParseArgs(NTrim(line + 4), args, 12);
        uint8_t r = (uint8_t)args[8], g = (uint8_t)args[9], b = (uint8_t)args[10];
        uint8_t f = (pc >= 12) ? (uint8_t)args[11] : 0;
        if (f) {
            // Trywialne wypelnienie (tymczasowo obejscie dla 2x Triangle)
            OutError("Wypelnienie QUAD (f=1) na razie niedostepne.");
        } else {
            DrawLine(args[0], args[1], args[2], args[3], r, g, b);
            DrawLine(args[2], args[3], args[4], args[5], r, g, b);
            DrawLine(args[4], args[5], args[6], args[7], r, g, b);
            DrawLine(args[6], args[7], args[0], args[1], r, g, b);
        }
        return;
    }

    // --- PENTAGON x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, r, g, b, [f] ---
    if (NStartsWith(line, "PENTAGON")) {
        int32_t args[14] = {0};
        int pc = ParseArgs(NTrim(line + 8), args, 14);
        uint8_t r = (uint8_t)args[10], g = (uint8_t)args[11], b = (uint8_t)args[12];
        uint8_t f = (pc >= 14) ? (uint8_t)args[13] : 0;
        if (f) {
            OutError("Wypelnienie PENTAGON (f=1) na razie niedostepne.");
        } else {
            DrawLine(args[0], args[1], args[2], args[3], r, g, b);
            DrawLine(args[2], args[3], args[4], args[5], r, g, b);
            DrawLine(args[4], args[5], args[6], args[7], r, g, b);
            DrawLine(args[6], args[7], args[8], args[9], r, g, b);
            DrawLine(args[8], args[9], args[0], args[1], r, g, b);
        }
        return;
    }

    // --- EGG x, y, rx, ry, angle, r, g, b, [f] ---
    if (NStartsWith(line, "EGG")) {
        int32_t args[9] = {0};
        int pc = ParseArgs(NTrim(line + 3), args, 9);
        int xc = args[0], yc = args[1], rx = args[2], ry = args[3], angle = args[4];
        uint8_t cr = (uint8_t)args[5], cg = (uint8_t)args[6], cb = (uint8_t)args[7];
        uint8_t f = (pc >= 9) ? (uint8_t)args[8] : 0;
        if (rx > 0 && ry > 0) {
            int max_rx = (rx * 13) / 10;
            int R = max_rx > ry ? max_rx : ry;
            R++; // zapas na obrot
            int s = NSin(angle);
            int c = NCos(angle);
            for (int sy = -R; sy <= R; sy++) {
                for (int sx = -R; sx <= R; sx++) {
                    // rotacja punktu ekranu do wnetrza jajka
                    int lx = (sx * c + sy * s) / 1000;
                    int ly = (-sx * s + sy * c) / 1000;
                    if (ly >= -ry && ly <= ry) {
                        int eff_rx = (rx * (ry * 10 + ly * 3)) / (ry * 10); // Gruby koniec zawsze na dole (dodatnie Y)
                        if (eff_rx < 1) eff_rx = 1;
                        int64_t term_x = ((int64_t)lx * 1000) / eff_rx;
                        int64_t term_y = ((int64_t)ly * 1000) / ry;
                        if (term_x * term_x + term_y * term_y <= 1000000LL) {
                            if (f || (term_x * term_x + term_y * term_y > 850000LL)) {
                                if (xc + sx >= 0 && yc + sy >= 0)
                                    HostRect((uint32_t)(xc + sx), (uint32_t)(yc + sy), 1, 1, cr, cg, cb);
                            }
                        }
                    }
                }
            }
        }
        return;
    }


    // --- TABLE_DATA nazwa, zrodlo ---
    if (NStartsWith(line, "TABLE_DATA")) {
        const char *p = NTrim(line + 10);
        char tname[64], sname[64];
        uint32_t tn = ParseIdent(p, tname, sizeof(tname));
        NyotaTable *t;
        NyotaVar *sv;
        if (!tname[0]) { OutError("TABLE_DATA: brak nazwy tabeli"); return; }
        p = NTrim(p + tn);
        if (*p != ',') { OutError("TABLE_DATA: wymagany przecinek"); return; }
        p = NTrim(p + 1);
        {
            uint32_t sn = ParseIdent(p, sname, sizeof(sname));
            if (!sname[0] || *NTrim(p + sn)) {
                OutError("TABLE_DATA: zrodlo musi byc nazwa zmiennej");
                return;
            }
        }
        t = FindTable(tname);
        if (!t) { OutError("TABLE_DATA: nieznana tabela"); return; }
        sv = FindVar(sname);
        if (!sv) { OutError("TABLE_DATA: zrodlo nie istnieje"); return; }
        if (!TableValidateSource(t, &sv->val)) return;
        NStrCopy(t->source_name, sname, sizeof(t->source_name));
        RenderTable(t);
        return;
    }

    // --- TABLE nazwa, x, y, w, h, kolumny, [szerokosci], "font", rozmiar, r, g, b [, zrodlo] ---
    if (PeekWord(line, "TABLE")) {
        const char *p = NTrim(line + 5);
        char tname[64];
        uint32_t tn = ParseIdent(p, tname, sizeof(tname));
        char args[12][MAX_STR_LEN];
        int n, i;
        int32_t x, y, w, h, cols, fsize, cr, cg, cb;
        NyotaVal widths, fontv;
        NyotaTable temp, *t;
        if (!g_is_graphics) { OutError("TABLE wymaga GRAPH"); return; }
        if (!tname[0]) { OutError("TABLE: brak nazwy tabeli"); return; }
        p = NTrim(p + tn);
        if (*p != ',') { OutError("TABLE: po nazwie wymagany przecinek"); return; }
        p = NTrim(p + 1);
        n = SplitFunctionArgs(p, args, 12);
        if (n != 11 && n != 12) {
            OutError("TABLE: wymagane 11 parametrow po nazwie i opcjonalne zrodlo");
            return;
        }
        if (!TableEvalInt(args[0], &x) || !TableEvalInt(args[1], &y) ||
            !TableEvalInt(args[2], &w) || !TableEvalInt(args[3], &h) ||
            !TableEvalInt(args[4], &cols) || !TableEvalInt(args[7], &fsize) ||
            !TableEvalInt(args[8], &cr) || !TableEvalInt(args[9], &cg) ||
            !TableEvalInt(args[10], &cb)) return;
        if (x < 0 || y < 0 || w <= 1 || h <= 1) {
            OutError("TABLE: nieprawidlowa pozycja lub rozmiar");
            return;
        }
        if (cols <= 0 || cols > MAX_TABLE_COLS) {
            OutError("TABLE: liczba kolumn poza zakresem 1..16");
            return;
        }
        if (fsize <= 0 || fsize > 64) {
            OutError("TABLE: rozmiar czcionki poza zakresem 1..64");
            return;
        }
        if (cr < 0 || cr > 255 || cg < 0 || cg > 255 || cb < 0 || cb > 255) {
            OutError("TABLE: kolor czcionki wymaga RGB 0..255");
            return;
        }
        widths = Eval(args[5]);
        if (widths.type != TYPE_LIST && widths.type != TYPE_TUPLE) {
            OutError("TABLE: szerokosci kolumn wymagaja LIST/TUPLE");
            return;
        }
        if (widths.list_len != (uint32_t)cols) {
            OutError("TABLE: liczba szerokosci nie zgadza sie z liczba kolumn");
            return;
        }
        fontv = Eval(args[6]);
        if (fontv.type != TYPE_STR || !fontv.s[0]) {
            OutError("TABLE: nazwa czcionki wymaga niepustego STRING");
            return;
        }
        temp.x = x; temp.y = y; temp.w = w; temp.h = h;
        temp.columns = (uint32_t)cols;
        temp.font_size = (uint32_t)fsize;
        temp.r = (uint8_t)cr; temp.g = (uint8_t)cg; temp.b = (uint8_t)cb;
        NStrCopy(temp.name, tname, sizeof(temp.name));
        NStrCopy(temp.font, fontv.s, sizeof(temp.font));
        temp.source_name[0] = '\0';
        {
            int32_t sum = 0;
            for (i = 0; i < cols; i++) {
                NyotaVal *wv = &widths.list_items[i];
                if (wv->type != TYPE_INT || wv->i <= 0) {
                    OutError("TABLE: kazda szerokosc kolumny wymaga dodatniego INTEGER");
                    return;
                }
                temp.widths[i] = wv->i;
                sum += wv->i;
            }
            if (sum != w) {
                OutError("TABLE: suma szerokosci kolumn musi byc rowna szerokosci tabeli");
                return;
            }
        }
        if (n == 12) {
            const char *sp = NTrim(args[11]);
            char sname[64];
            uint32_t sn = ParseIdent(sp, sname, sizeof(sname));
            NyotaVar *sv;
            if (!sname[0] || *NTrim(sp + sn)) {
                OutError("TABLE: zrodlo musi byc nazwa zmiennej");
                return;
            }
            sv = FindVar(sname);
            if (!sv) { OutError("TABLE: zrodlo danych nie istnieje"); return; }
            NStrCopy(temp.source_name, sname, sizeof(temp.source_name));
            if (!TableValidateSource(&temp, &sv->val)) return;
        }
        t = GetOrCreateTable(tname);
        if (!t) return;
        *t = temp;
        RenderTable(t);
        return;
    }

    // --- BUTTON nazwa, x, y, w, h, "tekst", "font", rozmiar, tr, tg, tb, br, bg, bb ---
    if (PeekWord(line, "BUTTON")) {
        const char *p = NTrim(line + 6);
        char bname[64];
        uint32_t bn = ParseIdent(p, bname, sizeof(bname));
        char args[14][MAX_STR_LEN];
        int n;
        int32_t x, y, w, h, fsize, tr, tg, tb, br, bg, bb;
        NyotaVal textv, fontv;
        NyotaButton temp, *b;
        if (!g_is_graphics) { OutError("BUTTON wymaga GRAPH"); return; }
        if (!bname[0]) { OutError("BUTTON: brak nazwy kontrolki"); return; }
        p = NTrim(p + bn);
        if (*p != ',') { OutError("BUTTON: po nazwie wymagany przecinek"); return; }
        p = NTrim(p + 1);
        n = SplitFunctionArgs(p, args, 14);
        if (n != 13) {
            OutError("BUTTON: wymagane x,y,w,h,tekst,font,rozmiar,RGB tekstu,RGB tla");
            return;
        }
        if (!ButtonEvalInt(args[0], &x) || !ButtonEvalInt(args[1], &y) ||
            !ButtonEvalInt(args[2], &w) || !ButtonEvalInt(args[3], &h) ||
            !ButtonEvalInt(args[6], &fsize) || !ButtonEvalInt(args[7], &tr) ||
            !ButtonEvalInt(args[8], &tg) || !ButtonEvalInt(args[9], &tb) ||
            !ButtonEvalInt(args[10], &br) || !ButtonEvalInt(args[11], &bg) ||
            !ButtonEvalInt(args[12], &bb)) return;
        if (x < 0 || y < 0 || w < 8 || h < 8) {
            OutError("BUTTON: nieprawidlowa pozycja lub rozmiar");
            return;
        }
        if (fsize <= 0 || fsize > 64) {
            OutError("BUTTON: rozmiar czcionki poza zakresem 1..64");
            return;
        }
        if (tr < 0 || tr > 255 || tg < 0 || tg > 255 || tb < 0 || tb > 255 ||
            br < 0 || br > 255 || bg < 0 || bg > 255 || bb < 0 || bb > 255) {
            OutError("BUTTON: kolory RGB wymagaja zakresu 0..255");
            return;
        }
        textv = Eval(args[4]);
        fontv = Eval(args[5]);
        if (textv.type != TYPE_STR) { OutError("BUTTON: tekst wymaga STRING"); return; }
        if (fontv.type != TYPE_STR || !fontv.s[0]) {
            OutError("BUTTON: nazwa czcionki wymaga niepustego STRING");
            return;
        }
        temp.x = x; temp.y = y; temp.w = w; temp.h = h;
        temp.font_size = (uint32_t)fsize;
        temp.text_r = (uint8_t)tr; temp.text_g = (uint8_t)tg; temp.text_b = (uint8_t)tb;
        temp.bg_r = (uint8_t)br; temp.bg_g = (uint8_t)bg; temp.bg_b = (uint8_t)bb;
        temp.prev_down = 0;
        NStrCopy(temp.name, bname, sizeof(temp.name));
        NStrCopy(temp.text, textv.s, sizeof(temp.text));
        NStrCopy(temp.font, fontv.s, sizeof(temp.font));
        b = GetOrCreateButton(bname);
        if (!b) return;
        *b = temp;
        RenderButton(b);
        return;
    }

    // --- SPRITE_ANIM nazwa, liczba_klatek, ms_na_klatke ---
    if (NStartsWith(line, "SPRITE_ANIM")) {
        const char *p = NTrim(line + 11);
        char sname[64], args[2][MAX_STR_LEN];
        uint32_t sn = ParseIdent(p, sname, sizeof(sname));
        NyotaSprite *s;
        int32_t frames, frame_ms;
        int n;
        if (!sname[0]) { OutError("SPRITE_ANIM: brak nazwy SPRITE"); return; }
        p = NTrim(p + sn);
        if (*p != ',') { OutError("SPRITE_ANIM: wymagany przecinek"); return; }
        n = SplitFunctionArgs(NTrim(p + 1), args, 2);
        if (n != 2) {
            OutError("SPRITE_ANIM wymaga liczby klatek i czasu klatki");
            return;
        }
        if (!SpriteEvalInt(args[0], &frames) || !SpriteEvalInt(args[1], &frame_ms)) return;
        if (frames < 1 || frames > 256) {
            OutError("SPRITE_ANIM: liczba klatek poza zakresem 1..256");
            return;
        }
        if (frame_ms < 1 || frame_ms > 60000) {
            OutError("SPRITE_ANIM: czas klatki poza zakresem 1..60000 ms");
            return;
        }
        s = FindSprite(sname);
        if (!s) { OutError("SPRITE_ANIM: nieznany SPRITE"); return; }
        if (frames > 1 && (!g_host || !g_host->gfx_sprite_draw_frame)) {
            OutError("SPRITE_ANIM: host nie obsluguje animacji sprite");
            return;
        }
        s->frame_count = (uint16_t)frames;
        s->frame = 0;
        s->frame_ms = (uint32_t)frame_ms;
        s->playing = frames > 1 ? 1 : 0;
        s->last_frame_tick = HostTicks();
        return;
    }

    // --- SPRITE_PLAY / SPRITE_STOP nazwa ---
    if (NStartsWith(line, "SPRITE_PLAY") || NStartsWith(line, "SPRITE_STOP")) {
        const char *p;
        char sname[64];
        uint32_t sn;
        NyotaSprite *s;
        int play = NStartsWith(line, "SPRITE_PLAY");
        p = NTrim(line + (play ? 11 : 11));
        sn = ParseIdent(p, sname, sizeof(sname));
        if (!sname[0] || *NTrim(p + sn)) {
            OutError("SPRITE_PLAY/STOP wymaga jednej nazwy SPRITE");
            return;
        }
        s = FindSprite(sname);
        if (!s) { OutError("SPRITE_PLAY/STOP: nieznany SPRITE"); return; }
        SpriteUpdateAnimation(s);
        if (play && s->frame_count > 1) {
            s->playing = 1;
            s->last_frame_tick = HostTicks();
        } else {
            s->playing = 0;
        }
        return;
    }

    // --- SPRITE_FRAME nazwa, indeks ---
    if (NStartsWith(line, "SPRITE_FRAME")) {
        const char *p = NTrim(line + 12);
        char sname[64], args[1][MAX_STR_LEN];
        uint32_t sn = ParseIdent(p, sname, sizeof(sname));
        NyotaSprite *s;
        int32_t frame;
        int n;
        if (!sname[0]) { OutError("SPRITE_FRAME: brak nazwy SPRITE"); return; }
        p = NTrim(p + sn);
        if (*p != ',') { OutError("SPRITE_FRAME: wymagany przecinek"); return; }
        n = SplitFunctionArgs(NTrim(p + 1), args, 1);
        if (n != 1) { OutError("SPRITE_FRAME wymaga indeksu klatki"); return; }
        if (!SpriteEvalInt(args[0], &frame)) return;
        s = FindSprite(sname);
        if (!s) { OutError("SPRITE_FRAME: nieznany SPRITE"); return; }
        if (frame < 0 || frame >= (int32_t)s->frame_count) {
            OutError("SPRITE_FRAME: indeks klatki poza zakresem");
            return;
        }
        s->frame = (uint16_t)frame;
        s->playing = 0;
        s->last_frame_tick = HostTicks();
        return;
    }

    // --- SPRITE_DRAW nazwa ---
    if (NStartsWith(line, "SPRITE_DRAW")) {
        const char *p = NTrim(line + 11);
        char sname[64];
        uint32_t sn = ParseIdent(p, sname, sizeof(sname));
        NyotaSprite *s;
        if (!sname[0] || *NTrim(p + sn)) {
            OutError("SPRITE_DRAW wymaga jednej nazwy SPRITE");
            return;
        }
        s = FindSprite(sname);
        if (!s) { OutError("SPRITE_DRAW: nieznany SPRITE"); return; }
        RenderSprite(s);
        return;
    }

    if (NStartsWith(line, "SPRITE_POS")) {
        const char *p = NTrim(line + 10);
        char sname[64], args[2][MAX_STR_LEN];
        uint32_t sn = ParseIdent(p, sname, sizeof(sname));
        NyotaSprite *s;
        int32_t x, y;
        int n;
        if (!sname[0]) { OutError("SPRITE_POS: brak nazwy SPRITE"); return; }
        p = NTrim(p + sn);
        if (*p != ',') { OutError("SPRITE_POS: wymagany przecinek"); return; }
        n = SplitFunctionArgs(NTrim(p + 1), args, 2);
        if (n != 2) { OutError("SPRITE_POS wymaga x, y"); return; }
        if (!SpriteEvalInt(args[0], &x) || !SpriteEvalInt(args[1], &y)) return;
        s = FindSprite(sname);
        if (!s) { OutError("SPRITE_POS: nieznany SPRITE"); return; }
        s->x = x; s->y = y;
        return;
    }

    if (NStartsWith(line, "SPRITE_MOVE")) {
        const char *p = NTrim(line + 11);
        char sname[64], args[2][MAX_STR_LEN];
        uint32_t sn = ParseIdent(p, sname, sizeof(sname));
        NyotaSprite *s;
        int32_t dx, dy;
        int n;
        if (!sname[0]) { OutError("SPRITE_MOVE: brak nazwy SPRITE"); return; }
        p = NTrim(p + sn);
        if (*p != ',') { OutError("SPRITE_MOVE: wymagany przecinek"); return; }
        n = SplitFunctionArgs(NTrim(p + 1), args, 2);
        if (n != 2) { OutError("SPRITE_MOVE wymaga dx, dy"); return; }
        if (!SpriteEvalInt(args[0], &dx) || !SpriteEvalInt(args[1], &dy)) return;
        s = FindSprite(sname);
        if (!s) { OutError("SPRITE_MOVE: nieznany SPRITE"); return; }
        s->x += dx; s->y += dy;
        return;
    }

    if (NStartsWith(line, "SPRITE_SHOW") || NStartsWith(line, "SPRITE_HIDE") ||
        NStartsWith(line, "SPRITE_DELETE")) {
        const char *p;
        char sname[64];
        uint32_t sn;
        int which;
        if (NStartsWith(line, "SPRITE_SHOW")) { p = NTrim(line + 11); which = 0; }
        else if (NStartsWith(line, "SPRITE_HIDE")) { p = NTrim(line + 11); which = 1; }
        else { p = NTrim(line + 13); which = 2; }
        sn = ParseIdent(p, sname, sizeof(sname));
        if (!sname[0] || *NTrim(p + sn)) {
            OutError("SPRITE_SHOW/HIDE/DELETE wymaga jednej nazwy SPRITE");
            return;
        }
        if (which == 2) {
            int32_t idx = FindSpriteIndex(sname);
            if (idx < 0) { OutError("SPRITE_DELETE: nieznany SPRITE"); return; }
            RemoveSpriteAt((uint32_t)idx);
        } else {
            NyotaSprite *s = FindSprite(sname);
            if (!s) { OutError("SPRITE_SHOW/HIDE: nieznany SPRITE"); return; }
            s->visible = (which == 0) ? 1 : 0;
        }
        return;
    }

    if (PeekWord(line, "SPRITE")) {
        const char *p = NTrim(line + 6);
        char sname[64], args[5][MAX_STR_LEN];
        uint32_t sn = ParseIdent(p, sname, sizeof(sname));
        NyotaVal source;
        NyotaSprite *s;
        int32_t x, y, w, h, handle;
        int n;
        if (!g_is_graphics) { OutError("SPRITE wymaga GRAPH"); return; }
        if (!sname[0]) { OutError("SPRITE: brak nazwy"); return; }
        p = NTrim(p + sn);
        if (*p != ',') { OutError("SPRITE: po nazwie wymagany przecinek"); return; }
        n = SplitFunctionArgs(NTrim(p + 1), args, 5);
        if (n != 5) {
            OutError("SPRITE: wymagane obraz, x, y, szerokosc, wysokosc");
            return;
        }
        source = Eval(args[0]);
        if (source.type != TYPE_STR || !source.s[0]) {
            OutError("SPRITE: obraz wymaga niepustego STRING");
            return;
        }
        if (!SpriteEvalInt(args[1], &x) || !SpriteEvalInt(args[2], &y) ||
            !SpriteEvalInt(args[3], &w) || !SpriteEvalInt(args[4], &h)) return;
        if (w <= 0 || h <= 0) {
            OutError("SPRITE: szerokosc i wysokosc musza byc dodatnie");
            return;
        }
        if (!g_host || !g_host->gfx_sprite_load || !g_host->gfx_sprite_draw) {
            OutError("SPRITE: host nie obsluguje sprite");
            return;
        }
        handle = HostSpriteLoad(source.s);
        if (handle < 0) {
            OutError("SPRITE: nie mozna zaladowac obrazu");
            return;
        }
        s = GetOrCreateSprite(sname);
        if (!s) { HostSpriteFree(handle); return; }
        if (s->host_handle >= 0) HostSpriteFree(s->host_handle);
        NStrCopy(s->source, source.s, sizeof(s->source));
        s->x = x; s->y = y; s->w = w; s->h = h;
        s->host_handle = handle;
        s->visible = 1;
        s->frame_count = 1;
        s->frame = 0;
        s->frame_ms = 100;
        s->playing = 0;
        s->last_frame_tick = 0;
        return;
    }

    // --- WITH rekord: ---
    if (NStartsWith(line, "WITH")) {
        const char *p = NTrim(line + 4);
        char name[64];
        uint32_t n = ParseIdent(p, name, sizeof(name));
        NyotaVar *v;
        NyotaVar *saved;
        uint32_t my_indent, body_start, body_end;
        p = NTrim(p + n);
        if (!name[0] || *p != ':' || *NTrim(p + 1)) {
            OutError("WITH wymaga WITH nazwa_rekordu:");
            return;
        }
        v = FindVar(name);
        if (!v || v->val.type != TYPE_RECORD) {
            OutError("WITH wymaga zmiennej RECORD");
            return;
        }
        my_indent = NIndent(raw);
        body_start = ln + 1;
        body_end = SkipBlock(body_start, my_indent);
        saved = g_with_var;
        g_with_var = v;
        ExecLines(body_start, body_end, my_indent + NYOTA_INDENT);
        g_with_var = saved;
        g_cur_line = body_end;
        return;
    }

    // --- FILE_WRITE / FILE_APPEND ---
    if (NStartsWith(line, "FILE_WRITE") || NStartsWith(line, "FILE_APPEND")) {
        char args[2][MAX_STR_LEN], path[MAX_STR_LEN];
        NyotaVal data;
        int append = NStartsWith(line, "FILE_APPEND");
        int off = append ? 11 : 10;
        int n = SplitFunctionArgs(NTrim(line + off), args, 2);
        if (n != 2 || !EvalStringArg(args[0], path, sizeof(path), append ? "FILE_APPEND" : "FILE_WRITE")) return;
        data = Eval(args[1]);
        if (data.type != TYPE_STR) { OutError("FILE_WRITE/APPEND: dane wymagaja STRING"); return; }
        if (!g_host || !g_host->file_write) { OutError("FILE_WRITE/APPEND: host nie obsluguje plikow"); return; }
        if (HostFileWrite(path, data.s, NStrLen(data.s), append ? 1 : 0) != 0)
            OutError("FILE_WRITE/APPEND: operacja nieudana");
        return;
    }

    // --- FILE_DELETE path ---
    if (NStartsWith(line, "FILE_DELETE")) {
        char args[1][MAX_STR_LEN], path[MAX_STR_LEN];
        int n = SplitFunctionArgs(NTrim(line + 11), args, 1);
        if (n != 1 || !EvalStringArg(args[0], path, sizeof(path), "FILE_DELETE")) return;
        if (!g_host || !g_host->file_delete) { OutError("FILE_DELETE: host nie obsluguje plikow"); return; }
        if (HostFileDelete(path) != 0) OutError("FILE_DELETE: operacja nieudana");
        return;
    }

    // --- FILE_COPY / FILE_MOVE src, dst ---
    if (NStartsWith(line, "FILE_COPY") || NStartsWith(line, "FILE_MOVE")) {
        char args[2][MAX_STR_LEN], src[MAX_STR_LEN], dst[MAX_STR_LEN];
        int move = NStartsWith(line, "FILE_MOVE");
        int n = SplitFunctionArgs(NTrim(line + 9), args, 2);
        if (n != 2 || !EvalStringArg(args[0], src, sizeof(src), move ? "FILE_MOVE" : "FILE_COPY") ||
            !EvalStringArg(args[1], dst, sizeof(dst), move ? "FILE_MOVE" : "FILE_COPY")) return;
        if (move) {
            if (!g_host || !g_host->file_move) { OutError("FILE_MOVE: host nie obsluguje plikow"); return; }
            if (HostFileMove(src, dst) != 0) OutError("FILE_MOVE: operacja nieudana");
        } else {
            if (!g_host || !g_host->file_copy) { OutError("FILE_COPY: host nie obsluguje plikow"); return; }
            if (HostFileCopy(src, dst) != 0) OutError("FILE_COPY: operacja nieudana");
        }
        return;
    }

    // --- DIR_CREATE / DIR_DELETE path ---
    if (NStartsWith(line, "DIR_CREATE") || NStartsWith(line, "DIR_DELETE")) {
        char args[1][MAX_STR_LEN], path[MAX_STR_LEN];
        int del = NStartsWith(line, "DIR_DELETE");
        int n = SplitFunctionArgs(NTrim(line + 10), args, 1);
        if (n != 1 || !EvalStringArg(args[0], path, sizeof(path), del ? "DIR_DELETE" : "DIR_CREATE")) return;
        if (del) {
            if (!g_host || !g_host->dir_delete) { OutError("DIR_DELETE: host nie obsluguje katalogow"); return; }
            if (HostDirDelete(path) != 0) OutError("DIR_DELETE: katalog musi byc pusty i istniec");
        } else {
            if (!g_host || !g_host->dir_create) { OutError("DIR_CREATE: host nie obsluguje katalogow"); return; }
            if (HostDirCreate(path) != 0) OutError("DIR_CREATE: operacja nieudana");
        }
        return;
    }

    // --- DIR_COPY / DIR_MOVE src, dst ---
    if (NStartsWith(line, "DIR_COPY") || NStartsWith(line, "DIR_MOVE")) {
        char args[2][MAX_STR_LEN], src[MAX_STR_LEN], dst[MAX_STR_LEN];
        int move = NStartsWith(line, "DIR_MOVE");
        int n = SplitFunctionArgs(NTrim(line + 8), args, 2);
        if (n != 2 || !EvalStringArg(args[0], src, sizeof(src), move ? "DIR_MOVE" : "DIR_COPY") ||
            !EvalStringArg(args[1], dst, sizeof(dst), move ? "DIR_MOVE" : "DIR_COPY")) return;
        if (move) {
            if (!g_host || !g_host->dir_move) { OutError("DIR_MOVE: host nie obsluguje katalogow"); return; }
            if (HostDirMove(src, dst) != 0) OutError("DIR_MOVE: operacja nieudana");
        } else {
            if (!g_host || !g_host->dir_copy) { OutError("DIR_COPY: host nie obsluguje katalogow"); return; }
            if (HostDirCopy(src, dst) != 0) OutError("DIR_COPY: operacja nieudana");
        }
        return;
    }

    // --- NYASM / ASM: bezpieczna maszyna wirtualna R0..R3 ---
    if (NStartsWith(line, "NYASM") || NStartsWith(line, "ASM")) {
        ExecNyasmBlock(ln, raw, line);
        return;
    }

    // --- VAR ---
    if (NStartsWith(line, "VAR")) {
        const char *p = NTrim(line + 3);
        char name[64]; uint32_t nlen = ParseIdent(p, name, sizeof(name));
        p = NTrim(p + nlen);
        if (NStrEq(name, "ERR_CODE")) { OutError("ERR_CODE jest zmienna systemowa tylko do odczytu"); return; }
        if (p[0] != ':' || p[1] != '=') { OutError("VAR: brakuje :="); return; }
        p = NTrim(p + 2);
        NyotaVal val = Eval(p);
        NyotaVar *v = FindVar(name);
        if (v && v->scope == g_call_depth) {
            // Sprawdź blokowanie typu tylko w obrębie bieżącego zasięgu
            if (v->val.type != TYPE_NONE && v->val.type != val.type) {
                OutError("Blokowanie typu: zmiana typu zmiennej zabroniona");
                return;
            }
        } else {
            // Jeśli zmienna nie istnieje lub jest w globalnym zasięgu -> tworzymy nową lokalną
            v = CreateVar(name);
            if (!v) { OutError("Za duzo zmiennych"); return; }
        }
        v->val = val;
        return;
    }

    // --- CONST ---
    if (NStartsWith(line, "CONST")) {
        const char *p = NTrim(line + 5);
        char name[64]; uint32_t nlen = ParseIdent(p, name, sizeof(name));
        p = NTrim(p + nlen);
        if (NStrEq(name, "ERR_CODE")) { OutError("ERR_CODE jest zarezerwowane"); return; }
        if (p[0] != ':' || p[1] != '=') { OutError("CONST: brakuje :="); return; }
        p = NTrim(p + 2);
        NyotaVal val = Eval(p);
        NyotaVar *v = GetOrCreateVar(name);
        if (!v) { OutError("Za duzo zmiennych"); return; }
        v->val = val;
        v->is_const = 1;
        return;
    }

    // --- Przypisanie (nazwa := wartość) ---
    if (NIsAlpha(line[0])) {
        // Sprawdź czy jest :=
        char name[64]; uint32_t nlen = ParseIdent(line, name, sizeof(name));
        const char *after = NTrim(line + nlen);

        // Przypisanie pola RECORD: obiekt.pole := wartosc
        if (*after == '.') {
            char field[64];
            uint32_t fn;
            int idx;
            NyotaVar *v = FindVar(name);
            NyotaVal rhs;
            after = NTrim(after + 1);
            fn = ParseIdent(after, field, sizeof(field));
            if (!v || v->val.type != TYPE_RECORD) { OutError("Dostep . wymaga RECORD"); return; }
            if (v->is_const) { OutError("Nie mozna zmienic stalego RECORD"); return; }
            if (!field[0]) { OutError("RECORD: brak nazwy pola"); return; }
            idx = RecordFieldIndex(&v->val, field);
            if (idx < 0) { OutError("RECORD: nieznane pole"); return; }
            after = NTrim(after + fn);
            if (after[0] != ':' || after[1] != '=') { OutError("RECORD: pole wymaga :="); return; }
            rhs = Eval(NTrim(after + 2));
            if (rhs.type == TYPE_NONE) return;
            if (v->val.list_items[idx].type != rhs.type) {
                OutError("RECORD: blokowanie typu pola");
                return;
            }
            v->val.list_items[idx] = rhs;
            return;
        }

        // Indeksowane przypisanie: lista[i] :=  albo  mark[klucz] :=  albo  mark[k][c] :=
        if (*after == '[') {
            char idx1[128];
            char idx2[128];
            uint32_t ii = 0, depth = 0, in_str = 0;
            idx2[0] = '\0';
            after++;
            while (*after && ii + 1 < sizeof(idx1)) {
                if (*after == '"') in_str = !in_str;
                else if (!in_str && *after == '[') depth++;
                else if (!in_str && *after == ']') {
                    if (depth == 0) break;
                    depth--;
                }
                idx1[ii++] = *after++;
            }
            idx1[ii] = '\0';
            if (*after == ']') after++;
            after = NTrim(after);
            if (*after == '[') {
                after++;
                ii = 0; depth = 0; in_str = 0;
                while (*after && ii + 1 < sizeof(idx2)) {
                    if (*after == '"') in_str = !in_str;
                    else if (!in_str && *after == '[') depth++;
                    else if (!in_str && *after == ']') {
                        if (depth == 0) break;
                        depth--;
                    }
                    idx2[ii++] = *after++;
                }
                idx2[ii] = '\0';
                if (*after == ']') after++;
                after = NTrim(after);
            }
            if (after[0] == ':' && after[1] == '=') {
                NyotaVar *v = FindVar(name);
                NyotaVal rhs = Eval(NTrim(after + 2));
                if (!v) { OutError("Zmienna niezadeklarowana"); return; }
                if (v->val.type == TYPE_MARK && v->is_const) { OutError("Nie mozna zmienic stalej MARK"); return; }
                if (v->val.type == TYPE_TUPLE) {
                    OutError("TUPLE jest niemutowalne");
                } else if (v->val.type == TYPE_LIST && !idx2[0]) {
                    NyotaVal idx = Eval(idx1);
                    int32_t i;
                    if (v->is_const) { OutError("Nie mozna zmienic stalej LIST"); return; }
                    if (idx.type != TYPE_INT) { OutError("Indeks LIST wymaga INTEGER"); return; }
                    i = idx.i;
                    if (i >= 0 && (uint32_t)i < v->val.list_len)
                        v->val.list_items[i] = rhs;
                    else OutError("Indeks listy poza zakresem");
                } else if (v->val.type == TYPE_MARK && !idx2[0]) {
                    NyotaVal key = Eval(idx1);
                    int r = FindMarkRow(&v->val, &key);
                    if (rhs.type != TYPE_LIST) { OutError("Wiersz MARK wymaga LIST"); return; }
                    if ((int32_t)rhs.list_len != v->val.i) {
                        OutError("LIST ma zla liczbe kolumn dla MARK");
                        return;
                    }
                    if (r < 0) {
                        NyotaVal *fields;
                        NyotaVal row;
                        uint32_t f;
                        if (v->val.list_len >= v->val.list_cap) {
                            OutError("MARK pelny");
                            return;
                        }
                        fields = PoolAlloc((uint32_t)v->val.i + 1);
                        if (!fields) { OutError("Pula list pelna"); return; }
                        ValClear(&row);
                        row.type = TYPE_LIST;
                        row.list_items = fields;
                        row.list_cap = (uint32_t)v->val.i + 1;
                        row.list_len = (uint32_t)v->val.i + 1;
                        row.list_items[0] = key;
                        for (f = 0; f < rhs.list_len; f++)
                            row.list_items[f + 1] = rhs.list_items[f];
                        v->val.list_items[v->val.list_len++] = row;
                    } else {
                        uint32_t f;
                        NyotaVal *row = &v->val.list_items[r];
                        for (f = 0; f < rhs.list_len && f + 1 < row->list_len; f++)
                            row->list_items[f + 1] = rhs.list_items[f];
                    }
                } else if (v->val.type == TYPE_MARK && idx2[0]) {
                    NyotaVal key = Eval(idx1);
                    NyotaVal colv = Eval(idx2);
                    int r = FindMarkRow(&v->val, &key);
                    int32_t c;
                    if (colv.type != TYPE_INT) { OutError("Kolumna MARK wymaga INTEGER"); return; }
                    c = colv.i;
                    if (r < 0) OutError("Brak klucza w MARK");
                    else if (c < 0 || c >= v->val.i) OutError("Kolumna MARK poza zakresem");
                    else v->val.list_items[r].list_items[c + 1] = rhs;
                } else {
                    OutError("Nieprawidlowe przypisanie indeksowane");
                }
            }
            return;
        }

        if (after[0] == ':' && after[1] == '=') {
            NyotaVal *wf;
            if (NStrEq(name, "ERR_CODE")) { OutError("ERR_CODE jest tylko do odczytu"); return; }
            wf = WithFieldPtr(name);
            if (wf) {
                NyotaVal nv;
                if (g_with_var && g_with_var->is_const) { OutError("Nie mozna zmienic stalego RECORD"); return; }
                nv = Eval(NTrim(after + 2));
                if (nv.type == TYPE_NONE) return;
                if (wf->type != nv.type) { OutError("RECORD: blokowanie typu pola"); return; }
                *wf = nv;
                return;
            }
            NyotaVar *v = FindVar(name);
            if (v) {
                if (v->is_const) { OutError("Nie mozna zmienic stałej"); return; }
                NyotaVal newval = Eval(NTrim(after + 2));
                if (v->val.type != TYPE_NONE && v->val.type != newval.type) {
                    OutError("Blokowanie typu: zmiana typu zabroniona");
                    return;
                }
                v->val = newval;
            } else {
                OutError("Zmienna niezadeklarowana (uzyj VAR)");
            }
            return;
        }

        // Wywołanie procedury/funkcji jako instrukcja
        if (*after == '(') {
            CallNamed(name, after, 0);
            return;
        }
    }

    // --- DELETE mark[klucz] ---
    if (NStartsWith(line, "DELETE")) {
        const char *p = NTrim(line + 6);
        char name[64];
        uint32_t nlen = ParseIdent(p, name, sizeof(name));
        p = NTrim(p + nlen);
        if (*p != '[') { OutError("DELETE wymaga mark[klucz]"); return; }
        p++;
        {
            char idx[128];
            uint32_t ii = 0, depth = 0, in_str = 0;
            while (*p && ii + 1 < sizeof(idx)) {
                if (*p == '"') in_str = !in_str;
                else if (!in_str && *p == '[') depth++;
                else if (!in_str && *p == ']') {
                    if (depth == 0) break;
                    depth--;
                }
                idx[ii++] = *p++;
            }
            idx[ii] = '\0';
            {
                NyotaVar *v = FindVar(name);
                NyotaVal key = Eval(idx);
                int r;
                if (!v || v->val.type != TYPE_MARK) { OutError("DELETE dziala tylko na MARK"); return; }
                if (v->is_const) { OutError("Nie mozna zmienic stalej MARK"); return; }
                r = FindMarkRow(&v->val, &key);
                if (r < 0) { OutError("DELETE: brak klucza"); return; }
                {
                    uint32_t j;
                    for (j = (uint32_t)r + 1; j < v->val.list_len; j++)
                        v->val.list_items[j - 1] = v->val.list_items[j];
                    v->val.list_len--;
                }
            }
        }
        return;
    }

    // --- REKEY mark, stary_klucz, nowy_klucz ---
    if (NStartsWith(line, "REKEY")) {
        char args[3][MAX_STR_LEN];
        int n = SplitFunctionArgs(NTrim(line + 5), args, 3);
        char name[64];
        const char *np;
        uint32_t nl;
        NyotaVar *v;
        NyotaVal oldk, newk;
        int row;
        if (n != 3) { OutError("REKEY mark, stary_klucz, nowy_klucz"); return; }
        np = NTrim(args[0]); nl = ParseIdent(np, name, sizeof(name));
        if (!name[0] || *NTrim(np + nl)) { OutError("REKEY: pierwszy argument musi byc nazwa MARK"); return; }
        v = FindVar(name);
        if (!v || v->val.type != TYPE_MARK) { OutError("REKEY wymaga MARK"); return; }
        if (v->is_const) { OutError("Nie mozna zmienic stalej MARK"); return; }
        oldk = Eval(args[1]); newk = Eval(args[2]);
        row = FindMarkRow(&v->val, &oldk);
        if (row < 0) { OutError("REKEY: brak starego klucza"); return; }
        if (ValEqual(&oldk, &newk)) return;
        if (FindMarkRow(&v->val, &newk) >= 0) { OutError("REKEY: nowy klucz juz istnieje"); return; }
        v->val.list_items[row].list_items[0] = newk;
        return;
    }

    // --- IF / ELIF / ELSE jako jeden łańcuch ---
    if (NStartsWith(line, "IF")) {
        uint32_t my_indent = NIndent(raw);
        uint32_t pos = ln;
        int taken = 0;
        int saw_else = 0;

        while (pos < g_line_count) {
            const char *raw2 = g_lines[pos];
            const char *cl = NTrim(raw2);
            if (!*cl || *cl == '#') { pos++; continue; }
            uint32_t ci = NIndent(raw2);
            if (ci != my_indent) break;

            int is_if = NStartsWith(cl, "IF");
            int is_elif = NStartsWith(cl, "ELIF");
            int is_else = NStartsWith(cl, "ELSE");
            if (pos == ln) {
                if (!is_if) break;
            } else {
                if (is_if || (!is_elif && !is_else)) break;
            }
            if (saw_else) {
                OutError("ELIF/ELSE po ELSE");
                break;
            }
            if (is_else) saw_else = 1;

            char cond[256];
            cond[0] = '\0';
            if (!is_else) {
                uint32_t kw_len = is_if ? 2 : 4;
                const char *cs = NTrim(cl + kw_len);
                NStrCopy(cond, cs, sizeof(cond));
                uint32_t clen = NStrLen(cond);
                if (clen > 0 && cond[clen - 1] == ':') cond[--clen] = '\0';
                NRTrim(cond);
            }

            uint32_t body_start = pos + 1;
            uint32_t body_end = SkipBlock(body_start, ci);
            int run = 0;
            if (!taken) {
                if (is_else) run = 1;
                else run = EvalBool(cond) ? 1 : 0;
            }
            if (run) {
                ExecLines(body_start, body_end, ci + NYOTA_INDENT);
                taken = 1;
            }
            pos = body_end;
            if (is_else) break;
        }
        g_cur_line = pos;
        return;
    }

    if (NStartsWith(line, "ELIF") || NStartsWith(line, "ELSE")) {
        OutError("ELIF/ELSE bez IF");
        g_cur_line = SkipBlock(ln + 1, NIndent(raw));
        return;
    }

    // --- FOR x IN lista:  albo  FOR i := START TO END [STEP n]: ---
    if (NStartsWith(line, "FOR")) {
        const char *p = NTrim(line + 3);
        char var_name[64]; uint32_t vlen = ParseIdent(p, var_name, sizeof(var_name));
        p = NTrim(p + vlen);
        if (PeekWord(p, "IN")) {
            char seq_expr[MAX_LINE_LEN];
            NyotaVal seq;
            NyotaVar *v;
            uint32_t my_indent, body_start, body_end, li;
            NStrCopy(seq_expr, NTrim(p + 2), sizeof(seq_expr));
            {
                uint32_t len = NStrLen(seq_expr);
                if (len > 0 && seq_expr[len - 1] == ':') seq_expr[--len] = '\0';
                NRTrim(seq_expr);
            }
            seq = Eval(seq_expr);
            if (seq.type != TYPE_LIST && seq.type != TYPE_TUPLE && seq.type != TYPE_MARK) { OutError("FOR ... IN wymaga LIST, TUPLE albo MARK"); return; }
            v = FindVar(var_name);
            if (v && v->is_const) { OutError("FOR ... IN nie moze uzyc CONST jako iteratora"); return; }
            if (!v) v = CreateVar(var_name);
            if (!v) { OutError("Za duzo zmiennych"); return; }
            my_indent = NIndent(raw);
            body_start = ln + 1;
            body_end = SkipBlock(body_start, my_indent);
            g_loop_depth++;
            for (li = 0; li < seq.list_len; li++) {
                if (seq.type == TYPE_MARK) v->val = seq.list_items[li].list_items[0];
                else v->val = seq.list_items[li];
                g_continue_flag = 0;
                ExecLines(body_start, body_end, my_indent + NYOTA_INDENT);
                if (g_continue_flag) { g_continue_flag = 0; continue; }
                if (g_exit_flag) { g_exit_flag = 0; break; }
                if (g_return_flag) break;
            }
            g_loop_depth--;
            g_cur_line = body_end;
            return;
        }
        if (p[0] != ':' || p[1] != '=') { OutError("FOR: brakuje := albo IN"); return; }
        p = NTrim(p + 2);
        const char *to_ptr = 0;
        uint32_t i;
        for (i = 0; p[i]; i++) {
            if (NStrEqN(p + i, " TO ", 4)) { to_ptr = p + i + 4; break; }
        }
        if (!to_ptr) { OutError("FOR: brakuje TO"); return; }
        char from_s[64]; uint32_t flen = (uint32_t)(to_ptr - p - 4);
        if (flen >= 64) flen = 63;
        for (i = 0; i < flen; i++) from_s[i] = p[i];
        from_s[flen] = '\0';
        NRTrim(from_s);

        char rest[128];
        NStrCopy(rest, to_ptr, sizeof(rest));
        uint32_t rlen = NStrLen(rest);
        if (rlen > 0 && rest[rlen - 1] == ':') rest[--rlen] = '\0';
        NRTrim(rest);

        char to_s[64];
        char step_s[64];
        step_s[0] = '\0';
        {
            const char *sp = 0;
            for (i = 0; rest[i]; i++) {
                if (NStrEqN(rest + i, " STEP ", 6)) { sp = rest + i + 6; break; }
            }
            if (sp) {
                uint32_t tlen = (uint32_t)(sp - rest - 6);
                if (tlen >= sizeof(to_s)) tlen = sizeof(to_s) - 1;
                for (i = 0; i < tlen; i++) to_s[i] = rest[i];
                to_s[tlen] = '\0';
                NRTrim(to_s);
                NStrCopy(step_s, sp, sizeof(step_s));
                NRTrim(step_s);
            } else {
                NStrCopy(to_s, rest, sizeof(to_s));
            }
        }

        NyotaVal from_val = Eval(from_s);
        NyotaVal to_val   = Eval(to_s);
        int32_t from_v = ValToInt(&from_val);
        int32_t to_v   = ValToInt(&to_val);
        int32_t step = (from_v <= to_v) ? 1 : -1;
        if (step_s[0]) {
            NyotaVal sv = Eval(step_s);
            if (sv.type != TYPE_INT) { OutError("STEP wymaga INTEGER"); return; }
            step = sv.i;
            if (step == 0) { OutError("STEP nie moze byc 0"); return; }
        }

        NyotaVar *v = GetOrCreateVar(var_name);
        if (!v) { OutError("Za duzo zmiennych"); return; }
        ValFromInt(&v->val, from_v);

        uint32_t my_indent = NIndent(raw);
        uint32_t body_start = ln + 1;
        uint32_t body_end = SkipBlock(body_start, my_indent);

        g_loop_depth++;
        for (int32_t it = from_v; step > 0 ? it <= to_v : it >= to_v; it += step) {
            v->val.i = it;
            g_continue_flag = 0;
            ExecLines(body_start, body_end, my_indent + NYOTA_INDENT);
            if (g_continue_flag) { g_continue_flag = 0; continue; }
            if (g_exit_flag) { g_exit_flag = 0; break; }
            if (g_return_flag) break;
        }
        g_loop_depth--;
        g_cur_line = body_end;
        return;
    }

    // --- WHILE cond: ---
    if (NStartsWith(line, "WHILE")) {
        char cond[256];
        NStrCopy(cond, NTrim(line + 5), sizeof(cond));
        {
            uint32_t clen = NStrLen(cond);
            if (clen > 0 && cond[clen - 1] == ':') cond[--clen] = '\0';
            NRTrim(cond);
        }
        uint32_t my_indent = NIndent(raw);
        uint32_t body_start = ln + 1;
        uint32_t body_end = SkipBlock(body_start, my_indent);
        g_loop_depth++;
        while (1) {
            NyotaVal cv = Eval(cond);
            if (cv.type != TYPE_BOOL) {
                OutError("WHILE wymaga BOOLEAN");
                break;
            }
            if (!cv.i) break;
            g_continue_flag = 0;
            ExecLines(body_start, body_end, my_indent + NYOTA_INDENT);
            if (g_continue_flag) { g_continue_flag = 0; continue; }
            if (g_exit_flag) { g_exit_flag = 0; break; }
            if (g_return_flag) break;
        }
        g_loop_depth--;
        g_cur_line = body_end;
        return;
    }

    // --- REPEAT ... UNTIL ---
    if (NStartsWith(line, "REPEAT")) {
        uint32_t my_indent = NIndent(raw);
        uint32_t body_start = ln + 1;
        uint32_t body_end = SkipBlock(body_start, my_indent);
        g_loop_depth++;
        while (1) {
            g_continue_flag = 0;
            ExecLines(body_start, body_end, my_indent + NYOTA_INDENT);
            if (g_continue_flag) g_continue_flag = 0;
            if (g_exit_flag) { g_exit_flag = 0; break; }
            if (g_return_flag) break;
            // Sprawdź UNTIL
            if (body_end < g_line_count) {
                const char *ul = NTrim(g_lines[body_end]);
                if (NStartsWith(ul, "UNTIL")) {
                    char cond[256]; NStrCopy(cond, NTrim(ul + 5), sizeof(cond));
                    if (EvalBool(cond)) break;
                }
            } else break;
        }
        g_loop_depth--;
        g_cur_line = body_end + 1;
        return;
    }

    // --- DO (nieskończona pętla) ---
    if (NStrEq(line, "DO:") || NStrEq(line, "DO")) {
        uint32_t my_indent = NIndent(raw);
        uint32_t body_start = ln + 1;
        uint32_t body_end = SkipBlock(body_start, my_indent);
        g_loop_depth++;
        while (!g_exit_flag && !g_return_flag) {
            g_continue_flag = 0;
            ExecLines(body_start, body_end, my_indent + NYOTA_INDENT);
            if (g_continue_flag) { g_continue_flag = 0; continue; }
            if (g_exit_flag) { g_exit_flag = 0; break; }
            if (g_return_flag) break;
        }
        g_loop_depth--;
        g_cur_line = body_end;
        return;
    }

    // --- EXIT ---
    if (NStrEq(line, "EXIT")) {
        if (g_loop_depth == 0) { OutError("EXIT poza petla"); return; }
        g_exit_flag = 1;
        return;
    }

    // --- CONTINUE ---
    if (NStrEq(line, "CONTINUE")) {
        if (g_loop_depth == 0) { OutError("CONTINUE poza petla"); return; }
        g_continue_flag = 1;
        return;
    }

    // --- RETURN ---
    if (NStartsWith(line, "RETURN")) {
        const char *p = NTrim(line + 6);
        if (g_call_depth == 0) {
            OutError("RETURN poza FUNCTION");
            return;
        }
        if (!g_in_function[g_call_depth]) {
            OutError("RETURN w PROCEDURE jest zabroniony");
            g_return_flag = 1;
            return;
        }
        if (!*p) {
            OutError("FUNCTION wymaga RETURN z wartoscia");
            g_return_flag = 1;
            return;
        }
        g_return_val = Eval(p);
        g_return_flag = 1;
        return;
    }

    // --- SWITCH / CASE ---
    if (NStartsWith(line, "SWITCH")) {
        const char *p = NTrim(line + 6);
        char sw_expr[256]; NStrCopy(sw_expr, p, sizeof(sw_expr));
        uint32_t slen = NStrLen(sw_expr);
        if (slen > 0 && sw_expr[slen-1] == ':') sw_expr[--slen] = '\0';
        NRTrim(sw_expr);
        NyotaVal sw_val = Eval(sw_expr);

        uint32_t my_indent = NIndent(raw);
        uint32_t i = ln + 1;
        int matched = 0;
        while (i < g_line_count) {
            const char *cl = NTrim(g_lines[i]);
            uint32_t ci = NIndent(g_lines[i]);
            if (ci <= my_indent && NStrLen(cl) > 0 && *cl != '#') break;
            if (!NStartsWith(cl, "CASE")) { i++; continue; }
            const char *cv = NTrim(cl + 4);
            char case_val[256]; NStrCopy(case_val, cv, sizeof(case_val));
            uint32_t cvlen = NStrLen(case_val);
            if (cvlen > 0 && case_val[cvlen-1] == ':') case_val[--cvlen] = '\0';
            NRTrim(case_val);

            uint32_t case_body = i + 1;
            uint32_t case_end = SkipBlock(case_body, ci);

            int is_default = NStrEq(case_val, "_");
            int case_match = is_default && !matched;
            if (!is_default && !matched) {
                NyotaVal cv2 = Eval(case_val);
                char ls[MAX_STR_LEN], rs[MAX_STR_LEN];
                ValToStr(&sw_val, ls, sizeof(ls));
                ValToStr(&cv2, rs, sizeof(rs));
                case_match = NStrEq(ls, rs);
            }
            if (case_match) {
                ExecLines(case_body, case_end, ci + NYOTA_INDENT);
                matched = 1;
            }
            i = case_end;
        }
        g_cur_line = i;
        return;
    }

    // --- PROCEDURE / FUNCTION (definicja — pomijamy podczas wykonania) ---
    if (NStartsWith(line, "PROCEDURE") || NStartsWith(line, "FUNCTION")) {
        uint32_t my_indent = NIndent(raw);
        g_cur_line = SkipBlock(ln + 1, my_indent);
        return;
    }

    // --- MEXTEND mark, count, default | defaults... ---
    if (NStartsWith(line, "MEXTEND")) {
        static char args[MAX_LIST_ITEMS + 2][MAX_STR_LEN];
        NyotaVal defs[MAX_LIST_ITEMS];
        int n = SplitFunctionArgs(NTrim(line + 7), args, MAX_LIST_ITEMS + 2);
        const char *np;
        char name[64];
        uint32_t nl, i, r;
        NyotaVar *v;
        NyotaVal cv;
        int32_t add;
        int defc;
        if (n < 3) { OutError("MEXTEND mark, count, default/defaults"); return; }
        np = NTrim(args[0]); nl = ParseIdent(np, name, sizeof(name));
        if (!name[0] || *NTrim(np + nl)) { OutError("MEXTEND: pierwszy argument musi byc nazwa MARK"); return; }
        v = FindVar(name);
        if (!v || v->val.type != TYPE_MARK) { OutError("MEXTEND wymaga MARK"); return; }
        if (v->is_const) { OutError("Nie mozna zmienic stalej MARK"); return; }
        cv = Eval(args[1]);
        if (cv.type != TYPE_INT || cv.i <= 0) { OutError("MEXTEND: count wymaga dodatniego INTEGER"); return; }
        add = cv.i; defc = n - 2;
        if (defc != 1 && defc != add) { OutError("MEXTEND: podaj jeden default albo po jednym na nowa kolumne"); return; }
        if (v->val.i + add > MAX_LIST_ITEMS - 1) { OutError("MEXTEND: za duzo kolumn dla biezacej implementacji"); return; }
        for (i = 0; i < (uint32_t)defc; i++) defs[i] = Eval(args[i + 2]);
        if (!MarkPreflightRows(v->val.list_len, (uint32_t)(v->val.i + add + 1))) return;
        for (r = 0; r < v->val.list_len; r++) {
            NyotaVal *row = &v->val.list_items[r];
            uint32_t old_len = row->list_len;
            uint32_t new_len = old_len + (uint32_t)add;
            NyotaVal *fields = PoolAlloc(new_len);
            if (!fields) { OutError("Pula list pelna"); return; }
            for (i = 0; i < old_len; i++) fields[i] = row->list_items[i];
            for (i = 0; i < (uint32_t)add; i++) fields[old_len + i] = defs[defc == 1 ? 0 : i];
            row->list_items = fields; row->list_len = new_len; row->list_cap = new_len;
        }
        v->val.i += add;
        return;
    }

    // --- MINSERT mark, index, default ---
    if (NStartsWith(line, "MINSERT")) {
        char args[3][MAX_STR_LEN], name[64];
        int n = SplitFunctionArgs(NTrim(line + 7), args, 3);
        const char *np;
        uint32_t nl, r, i;
        NyotaVar *v;
        NyotaVal iv, def;
        int32_t at;
        if (n != 3) { OutError("MINSERT mark, index, default"); return; }
        np = NTrim(args[0]); nl = ParseIdent(np, name, sizeof(name));
        if (!name[0] || *NTrim(np + nl)) { OutError("MINSERT: pierwszy argument musi byc nazwa MARK"); return; }
        v = FindVar(name);
        if (!v || v->val.type != TYPE_MARK) { OutError("MINSERT wymaga MARK"); return; }
        if (v->is_const) { OutError("Nie mozna zmienic stalej MARK"); return; }
        iv = Eval(args[1]); def = Eval(args[2]);
        if (iv.type != TYPE_INT) { OutError("MINSERT: index wymaga INTEGER"); return; }
        at = iv.i;
        if (at < 0 || at > v->val.i) { OutError("MINSERT: indeks poza zakresem"); return; }
        if (v->val.i >= MAX_LIST_ITEMS - 1) { OutError("MINSERT: za duzo kolumn dla biezacej implementacji"); return; }
        if (!MarkPreflightRows(v->val.list_len, (uint32_t)v->val.i + 2U)) return;
        for (r = 0; r < v->val.list_len; r++) {
            NyotaVal *row = &v->val.list_items[r];
            uint32_t new_len = row->list_len + 1U;
            NyotaVal *fields = PoolAlloc(new_len);
            if (!fields) { OutError("Pula list pelna"); return; }
            fields[0] = row->list_items[0];
            for (i = 0; i < (uint32_t)at; i++) fields[i + 1] = row->list_items[i + 1];
            fields[at + 1] = def;
            for (i = (uint32_t)at; i < (uint32_t)v->val.i; i++) fields[i + 2] = row->list_items[i + 1];
            row->list_items = fields; row->list_len = new_len; row->list_cap = new_len;
        }
        v->val.i++;
        return;
    }

    // --- MDROP mark, index ---
    if (NStartsWith(line, "MDROP")) {
        char args[2][MAX_STR_LEN], name[64];
        int n = SplitFunctionArgs(NTrim(line + 5), args, 2);
        const char *np;
        uint32_t nl, r, i;
        NyotaVar *v;
        NyotaVal iv;
        int32_t at;
        if (n != 2) { OutError("MDROP mark, index"); return; }
        np = NTrim(args[0]); nl = ParseIdent(np, name, sizeof(name));
        if (!name[0] || *NTrim(np + nl)) { OutError("MDROP: pierwszy argument musi byc nazwa MARK"); return; }
        v = FindVar(name);
        if (!v || v->val.type != TYPE_MARK) { OutError("MDROP wymaga MARK"); return; }
        if (v->is_const) { OutError("Nie mozna zmienic stalej MARK"); return; }
        iv = Eval(args[1]);
        if (iv.type != TYPE_INT) { OutError("MDROP: index wymaga INTEGER"); return; }
        at = iv.i;
        if (at < 0 || at >= v->val.i) { OutError("MDROP: indeks poza zakresem"); return; }
        for (r = 0; r < v->val.list_len; r++) {
            NyotaVal *row = &v->val.list_items[r];
            for (i = (uint32_t)at + 1U; i < (uint32_t)v->val.i; i++)
                row->list_items[i] = row->list_items[i + 1];
            row->list_len--;
        }
        v->val.i--;
        return;
    }

    // --- APPEND lista, element ---
    if (NStartsWith(line, "APPEND")) {
        const char *p = NTrim(line + 6);
        char list_name[64]; uint32_t nl = ParseIdent(p, list_name, sizeof(list_name));
        NyotaVar *v;
        NyotaVal item;
        if (!list_name[0]) { OutError("APPEND: brak nazwy LIST"); return; }
        p = NTrim(p + nl);
        if (*p != ',') { OutError("APPEND: wymagany przecinek"); return; }
        p = NTrim(p + 1);
        if (!*p) { OutError("APPEND: brak elementu"); return; }
        v = FindVar(list_name);
        if (!v || v->val.type != TYPE_LIST) { OutError("APPEND wymaga LIST"); return; }
        if (v->is_const) { OutError("Nie mozna zmienic stalej LIST"); return; }
        item = Eval(p);
        if (item.type == TYPE_NONE) return;
        if (!ListEnsureCapacity(&v->val, v->val.list_len + 1)) return;
        v->val.list_items[v->val.list_len++] = item;
        return;
    }

    // --- REMOVE lista[index]  albo  REMOVE lista, wartosc [, ALL] ---
    if (NStartsWith(line, "REMOVE")) {
        const char *p = NTrim(line + 6);
        char list_name[64]; uint32_t nl = ParseIdent(p, list_name, sizeof(list_name));
        NyotaVar *v = FindVar(list_name);
        p = NTrim(p + nl);
        if (!v || v->val.type != TYPE_LIST) { OutError("REMOVE wymaga LIST"); return; }
        if (v->is_const) { OutError("Nie mozna zmienic stalej LIST"); return; }
        if (*p == '[') {
            char idx_buf[128];
            uint32_t ii = 0;
            int depth = 0, in_str = 0;
            p++;
            while (*p && ii + 1 < sizeof(idx_buf)) {
                if (*p == '"') in_str = !in_str;
                else if (!in_str && *p == '[') depth++;
                else if (!in_str && *p == ']') {
                    if (depth == 0) break;
                    depth--;
                }
                idx_buf[ii++] = *p++;
            }
            idx_buf[ii] = '\0';
            if (*p != ']') { OutError("REMOVE: brak ]"); return; }
            p = NTrim(p + 1);
            if (*p) { OutError("REMOVE: nadmiarowa skladnia po indeksie"); return; }
            {
                NyotaVal idx = Eval(idx_buf);
                int32_t i;
                if (idx.type != TYPE_INT) { OutError("Indeks LIST wymaga INTEGER"); return; }
                i = idx.i;
                if (i < 0 || (uint32_t)i >= v->val.list_len) OutError("REMOVE: indeks poza zakresem");
                else ListRemoveAt(&v->val, (uint32_t)i);
            }
            return;
        }
        if (*p != ',') { OutError("REMOVE: wymagany indeks [] albo przecinek"); return; }
        p = NTrim(p + 1);
        {
            char args[2][MAX_STR_LEN];
            int n = SplitFunctionArgs(p, args, 2);
            int all = 0;
            NyotaVal val;
            if (n < 1 || n > 2 || !NTrim(args[0])[0]) { OutError("REMOVE: zle argumenty"); return; }
            if (n == 2) {
                if (!NStrEq(NTrim(args[1]), "ALL")) { OutError("REMOVE: trzeci argument moze byc tylko ALL"); return; }
                all = 1;
            }
            val = Eval(args[0]);
            if (val.type == TYPE_NONE) return;
            if (all) {
                int f;
                while ((f = ListFindEq(&v->val, &val)) >= 0)
                    ListRemoveAt(&v->val, (uint32_t)f);
            } else {
                int f = ListFindEq(&v->val, &val);
                if (f >= 0) ListRemoveAt(&v->val, (uint32_t)f);
                else OutError("REMOVE: brak wartosci");
            }
        }
        return;
    }

    // --- EXTEND a, b (LIST albo MARK) ---
    if (NStartsWith(line, "EXTEND")) {
        const char *p = NTrim(line + 6);
        char na[64], nb[64];
        uint32_t n1 = ParseIdent(p, na, sizeof(na));
        uint32_t n2;
        p = NTrim(p + n1);
        if (!na[0] || *p != ',') { OutError("EXTEND: wymagane EXTEND a, b"); return; }
        p = NTrim(p + 1);
        n2 = ParseIdent(p, nb, sizeof(nb));
        if (!nb[0] || *NTrim(p + n2)) { OutError("EXTEND: wymagane dwie nazwy kolekcji"); return; }
        {
            NyotaVar *a = FindVar(na);
            NyotaVar *b = FindVar(nb);
            uint32_t i, source_len;
            if (!a || !b || a->val.type != b->val.type ||
                (a->val.type != TYPE_LIST && a->val.type != TYPE_MARK)) {
                OutError("EXTEND wymaga dwoch LIST albo dwoch MARK");
                return;
            }
            if (a->is_const) { OutError("Nie mozna zmienic stalej kolekcji"); return; }
            if (a->val.type == TYPE_LIST) {
                source_len = b->val.list_len;
                for (i = 0; i < source_len; i++) {
                    if (ListFindEq(&a->val, &b->val.list_items[i]) >= 0) continue;
                    if (!ListEnsureCapacity(&a->val, a->val.list_len + 1)) return;
                    a->val.list_items[a->val.list_len++] = b->val.list_items[i];
                }
            } else {
                if (a->val.i != b->val.i) { OutError("EXTEND MARK wymaga zgodnej liczby kolumn wartosci"); return; }
                source_len = b->val.list_len;
                for (i = 0; i < source_len; i++) {
                    NyotaVal *row = &b->val.list_items[i];
                    if (FindMarkRow(&a->val, &row->list_items[0]) >= 0) continue;
                    if (!MarkAppendRowCopy(&a->val, row)) return;
                }
            }
        }
        return;
    }

    // --- REVERSE LIST/MARK ---
    if (NStartsWith(line, "REVERSE")) {
        const char *p = NTrim(line + 7);
        char name[64];
        uint32_t nlen = ParseIdent(p, name, sizeof(name));
        if (!name[0] || *NTrim(p + nlen)) { OutError("REVERSE wymaga jednej nazwy LIST/MARK"); return; }
        {
            NyotaVar *v = FindVar(name);
            uint32_t i, n;
            if (!v || (v->val.type != TYPE_LIST && v->val.type != TYPE_MARK)) { OutError("REVERSE wymaga LIST albo MARK"); return; }
            if (v->is_const) { OutError("Nie mozna zmienic stalej kolekcji"); return; }
            n = v->val.list_len;
            for (i = 0; i < n / 2; i++) {
                NyotaVal tmp = v->val.list_items[i];
                v->val.list_items[i] = v->val.list_items[n - 1 - i];
                v->val.list_items[n - 1 - i] = tmp;
            }
        }
        return;
    }

    // --- SORT LIST / SORT MARK ---
    if (NStartsWith(line, "SORT")) {
        const char *p = NTrim(line + 4);
        char name[64];
        uint32_t nlen = ParseIdent(p, name, sizeof(name));
        NyotaVar *v = FindVar(name);
        p = NTrim(p + nlen);
        if (!v) { OutError("SORT: nieznana zmienna"); return; }
        if (v->is_const) { OutError("Nie mozna zmienic stalej kolekcji"); return; }

        if (v->val.type == TYPE_MARK) {
            char args[2][MAX_STR_LEN];
            int n, desc = 0, by_key = 0;
            int32_t col = -1;
            if (*p != ',') { OutError("SORT MARK: uzyj KEY albo VALUE n"); return; }
            n = SplitFunctionArgs(NTrim(p + 1), args, 2);
            if (n < 1 || n > 2) { OutError("SORT MARK: zla skladnia"); return; }
            {
                const char *sel = NTrim(args[0]);
                if (NStrEq(sel, "KEY")) by_key = 1;
                else if (PeekWord(sel, "VALUE")) {
                    NyotaVal cv = Eval(NTrim(sel + 5));
                    if (cv.type != TYPE_INT) { OutError("SORT MARK VALUE wymaga indeksu INTEGER"); return; }
                    col = cv.i;
                } else { OutError("SORT MARK: uzyj KEY albo VALUE n"); return; }
            }
            if (n == 2) {
                const char *dir = NTrim(args[1]);
                if (NStrEq(dir, "DESC")) desc = 1;
                else if (NStrEq(dir, "ASC")) desc = 0;
                else { OutError("SORT MARK: uzyj ASC albo DESC"); return; }
            }
            MarkSort(&v->val, by_key, col, desc);
            return;
        }

        if (v->val.type == TYPE_LIST) {
            int desc = 0, algo = LIST_SORT_AUTO;
            int dir_seen = 0, algo_seen = 0;
            if (*p == ',') {
                char mods[2][MAX_STR_LEN];
                int n = SplitFunctionArgs(NTrim(p + 1), mods, 2);
                int mi;
                if (n < 1 || n > 2) { OutError("SORT: maksymalnie algorytm i kierunek"); return; }
                for (mi = 0; mi < n; mi++) {
                    const char *m = NTrim(mods[mi]);
                    if (NStrEq(m, "ASC")) {
                        if (dir_seen) { OutError("SORT: kierunek podany wiecej niz raz"); return; }
                        desc = 0; dir_seen = 1;
                    } else if (NStrEq(m, "DESC") || NStrEq(m, "REVERSE")) {
                        if (dir_seen) { OutError("SORT: kierunek podany wiecej niz raz"); return; }
                        desc = 1; dir_seen = 1;
                    } else {
                        int a = -1;
                        if (NStrEq(m, "AUTO")) a = LIST_SORT_AUTO;
                        else if (NStrEq(m, "BUBBLE")) a = LIST_SORT_BUBBLE;
                        else if (NStrEq(m, "INSERT")) a = LIST_SORT_INSERT;
                        else if (NStrEq(m, "SELECT")) a = LIST_SORT_SELECT;
                        else if (NStrEq(m, "MERGE")) a = LIST_SORT_MERGE;
                        else if (NStrEq(m, "QUICK")) a = LIST_SORT_QUICK;
                        else if (NStrEq(m, "HEAP")) a = LIST_SORT_HEAP;
                        else if (NStrEq(m, "SHELL")) a = LIST_SORT_SHELL;
                        else if (NStrEq(m, "COUNTING")) a = LIST_SORT_COUNTING;
                        else { OutError("SORT: nieznany algorytm/kierunek"); return; }
                        if (algo_seen) { OutError("SORT: algorytm podany wiecej niz raz"); return; }
                        algo = a; algo_seen = 1;
                    }
                }
            } else if (*p) {
                OutError("SORT: nadmiarowa skladnia"); return;
            }
            if (v->val.list_len > 0) {
                uint8_t t = v->val.list_items[0].type;
                uint32_t i;
                if (t != TYPE_INT && t != TYPE_FLOAT && t != TYPE_STR &&
                    t != TYPE_DATE && t != TYPE_TIME) {
                    OutError("SORT: obslugiwane typy to INTEGER, FLOAT, STRING, DATE, TIME");
                    return;
                }
                for (i = 1; i < v->val.list_len; i++) {
                    if (v->val.list_items[i].type != t) {
                        OutError("SORT: lista typow mieszanych");
                        return;
                    }
                }
            }
            ListSortAlgo(&v->val, desc, algo);
            return;
        }
        OutError("SORT wymaga LIST albo MARK");
        return;
    }

    // --- DELAY ms ---
    if (NStartsWith(line, "DELAY")) {
        const char *p = NTrim(line + 5);
        NyotaVal v = Eval(p);
        int32_t ms = ValToInt(&v);
        uint64_t start = HostTicks();
        // GetTicks to takty 100Hz — 100 taktów = 1000ms
        uint64_t wait = (uint64_t)ms / 10;
        while ((HostTicks() - start) < wait) {
            // busy wait
        }
        return;
    }

    // --- SCREEN id OPEN w, h, mode / SCREEN id SET ---
    if (NStartsWith(line, "SCREEN")) {
        const char *p = NTrim(line + 6);
        char idbuf[64];
        uint32_t bi = 0;
        NyotaVal idv;
        int32_t id;
        while (*p && !NIsSpace(*p) && bi + 1 < sizeof(idbuf)) idbuf[bi++] = *p++;
        idbuf[bi] = '\0';
        idv = Eval(idbuf);
        if (idv.type != TYPE_INT) { OutError("SCREEN: ID wymaga INTEGER"); return; }
        id = idv.i;
        if (id < 0 || id >= MAX_SCREENS) { OutError("SCREEN: ID poza zakresem 0..15"); return; }
        p = NTrim(p);
        if (PeekWord(p, "OPEN")) {
            char args[3][MAX_STR_LEN];
            int32_t w, h, mode;
            int n;
            if (!g_is_graphics) { OutError("SCREEN OPEN wymaga GRAPH"); return; }
            if (id == 0) { OutError("SCREEN 0 nie moze zostac otwarty ponownie"); return; }
            if (g_screen_exists[id]) { OutError("SCREEN: ID juz istnieje"); return; }
            n = SplitFunctionArgs(NTrim(p + 4), args, 3);
            if (n != 3) { OutError("SCREEN OPEN wymaga szerokosc, wysokosc, tryb_kolorow"); return; }
            {
                NyotaVal wv = Eval(args[0]), hv = Eval(args[1]), mv = Eval(args[2]);
                if (wv.type != TYPE_INT || hv.type != TYPE_INT || mv.type != TYPE_INT) {
                    OutError("SCREEN OPEN: parametry wymagaja INTEGER");
                    return;
                }
                w = wv.i; h = hv.i; mode = mv.i;
            }
            if (w <= 0 || h <= 0 || mode <= 0) { OutError("SCREEN OPEN: parametry musza byc dodatnie"); return; }
            if ((uint32_t)w != g_graph_w || (uint32_t)h != g_graph_h) {
                OutError("SCREEN OPEN: rozdzielczosc musi byc identyczna z GRAPH");
                return;
            }
            if (HostScreenOpen((uint32_t)id, (uint32_t)w, (uint32_t)h, (uint32_t)mode) != 0) {
                OutError("SCREEN OPEN: host nie mogl utworzyc celu off-screen");
                return;
            }
            g_screen_exists[id] = 1;
            return;
        }
        if (PeekWord(p, "SET")) {
            if (*NTrim(p + 3)) { OutError("SCREEN SET: nadmiarowa skladnia"); return; }
            if (id != 0 && !g_screen_exists[id]) { OutError("SCREEN SET: ekran nie istnieje"); return; }
            if (HostScreenSet((uint32_t)id) != 0) { OutError("SCREEN SET: host nie obsluguje celu"); return; }
            g_active_screen = (uint32_t)id;
            return;
        }
        OutError("SCREEN: oczekiwano OPEN albo SET");
        return;
    }

    // --- GRAPH id ---
    if (NStartsWith(line, "GRAPH")) {
        NyotaVal gv = Eval(NTrim(line + 5));
        int32_t id = ValToInt(&gv);
        uint32_t gw = 640, gh = 480;
        if (id == 1) { gw = 640; gh = 480; }
        else if (id == 2) { gw = 800; gh = 600; }
        else if (id == 3) { gw = 1024; gh = 768; }
        else if (id == 4) { gw = 1280; gh = 1024; }
        else if (id == 5) { gw = 1600; gh = 900; }
        else if (id == 6) { gw = 1920; gh = 1080; }
        else {
            OutError("GRAPH: tryb 1..6");
            return;
        }
        g_is_graphics = 1;
        g_graph_w = gw; g_graph_h = gh;
        {
            uint32_t si;
            for (si = 0; si < MAX_SCREENS; si++) g_screen_exists[si] = 0;
            g_screen_exists[0] = 1;
            g_active_screen = 0;
        }
        HostGfxMode(gw, gh);
        if (g_host && g_host->gfx_screen_set) HostScreenSet(0);
        return;
    }

    // --- EVERY ms CALL procedura ---
    if (NStartsWith(line, "EVERY")) {
        const char *p = NTrim(line + 5);
        const char *callp = 0;
        char interval_expr[128], pname[64];
        uint32_t k, len, pn;
        NyotaVal iv;
        NyotaProc *proc;
        uint64_t ticks;
        int slot = -1;
        for (k = 0; p[k]; k++) {
            if (NStrEqN(p + k, " CALL ", 6)) { callp = p + k; break; }
        }
        if (!callp) { OutError("EVERY: oczekiwano EVERY ms CALL Procedura"); return; }
        len = (uint32_t)(callp - p);
        if (len == 0 || len >= sizeof(interval_expr)) { OutError("EVERY: zly interwal"); return; }
        for (k = 0; k < len; k++) interval_expr[k] = p[k];
        interval_expr[len] = '\0'; NRTrim(interval_expr);
        iv = Eval(interval_expr);
        if (iv.type != TYPE_INT || iv.i <= 0) { OutError("EVERY: interwal musi byc dodatnim INTEGER w ms"); return; }
        p = NTrim(callp + 6);
        pn = ParseIdent(p, pname, sizeof(pname));
        if (!pname[0] || *NTrim(p + pn)) { OutError("EVERY: brak poprawnej nazwy procedury"); return; }
        proc = FindProcByName(pname);
        if (!proc || proc->is_function || proc->param_count != 0) {
            OutError("EVERY wymaga istniejacej PROCEDURE bez parametrow");
            return;
        }
        ticks = ((uint64_t)iv.i + 9ULL) / 10ULL;
        if (ticks < 1) ticks = 1;
        for (k = 0; k < MAX_EVERY_EVENTS; k++) {
            if (g_every[k].active && NStrEq(g_every[k].proc_name, pname)) { slot = (int)k; break; }
            if (slot < 0 && !g_every[k].active) slot = (int)k;
        }
        if (slot < 0) { OutError("EVERY: przekroczono limit 16 zdarzen"); return; }
        NStrCopy(g_every[slot].proc_name, pname, sizeof(g_every[slot].proc_name));
        g_every[slot].interval_ticks = ticks;
        g_every[slot].next_tick = HostTicks() + ticks;
        g_every[slot].active = 1;
        return;
    }

    // --- CANCEL EVERY procedura ---
    if (NStartsWith(line, "CANCEL EVERY")) {
        const char *p = NTrim(line + 12);
        char pname[64];
        uint32_t pn = ParseIdent(p, pname, sizeof(pname));
        uint32_t i;
        if (!pname[0] || *NTrim(p + pn)) { OutError("CANCEL EVERY: brak nazwy procedury"); return; }
        for (i = 0; i < MAX_EVERY_EVENTS; i++) {
            if (g_every[i].active && NStrEq(g_every[i].proc_name, pname)) {
                g_every[i].active = 0;
                return;
            }
        }
        return; /* anulowanie nieaktywnego zdarzenia jest idempotentne */
    }

    // --- STATE_CREATE id ---
    if (NStartsWith(line, "STATE_CREATE")) {
        int32_t args[1] = {0}; ParseArgs(NTrim(line + 12), args, 1);
        if (args[0] >= 0 && args[0] < MAX_STATES) {
            g_states_active[args[0]] = 1; g_states[args[0]][0] = '\0';
        } else OutError("STATE_CREATE: zly ID (max 31)");
        return;
    }

    // --- STATE_SET id, "stan" ---
    if (NStartsWith(line, "STATE_SET")) {
        char args[2][MAX_STR_LEN];
        if (SplitFunctionArgs(NTrim(line + 9), args, 2) == 2) {
            NyotaVal v0 = Eval(args[0]);
            int32_t id = ValToInt(&v0);
            if (id >= 0 && id < MAX_STATES && g_states_active[id]) {
                NyotaVal v1 = Eval(args[1]); ValToStr(&v1, g_states[id], 64);
            }
        }
        return;
    }

    // --- NN_CREATE id, inputs ---
    if (NStartsWith(line, "NN_CREATE")) {
        int32_t args[2] = {0};
        ParseArgs(NTrim(line + 9), args, 2);
        int32_t id = args[0], inputs = args[1];
        if (id >= 0 && id < MAX_NN && inputs > 0 && inputs <= MAX_NN_WEIGHTS) {
            g_nn[id].active = 1; g_nn[id].inputs = inputs; g_nn[id].bias = 0;
            for(int i=0; i<MAX_NN_WEIGHTS; i++) g_nn[id].weights[i] = 100;
        } else OutError("NN_CREATE: zly ID lub ilosc wejsc");
        return;
    }

    // --- NN_TRAIN id, list, expected ---
    if (NStartsWith(line, "NN_TRAIN")) {
        char args[3][MAX_STR_LEN];
        if (SplitFunctionArgs(NTrim(line + 8), args, 3) == 3) {
            NyotaVal v0 = Eval(args[0]);
            int32_t id = ValToInt(&v0);
            NyotaVal lst = Eval(args[1]);
            NyotaVal v2 = Eval(args[2]);
            int32_t expected = ValToInt(&v2);
            if (id >= 0 && id < MAX_NN && g_nn[id].active && lst.type == TYPE_LIST) {
                int32_t sum = g_nn[id].bias;
                uint32_t in_cnt = g_nn[id].inputs > lst.list_len ? lst.list_len : g_nn[id].inputs;
                for (uint32_t i=0; i<in_cnt; i++) sum += (ValToInt(&lst.list_items[i]) * g_nn[id].weights[i]) / 1000;
                if (sum < 0) sum = 0; if (sum > 1000) sum = 1000;
                int32_t error = expected - sum;
                int32_t lr = 25; // learning rate
                g_nn[id].bias += (error * lr) / 1000;
                for(uint32_t i=0; i<in_cnt; i++)
                    g_nn[id].weights[i] += (error * lr * ValToInt(&lst.list_items[i])) / 1000000;
            }
        }
        return;
    }

    // --- ON ERROR CALL procedura ---
    if (NStartsWith(line, "ON ERROR CALL")) {
        const char *p = NTrim(line + 13);
        char pname[64];
        uint32_t pn = ParseIdent(p, pname, sizeof(pname));
        uint32_t pi;
        if (!pname[0] || *NTrim(p + pn)) { OutError("ON ERROR CALL: brak nazwy procedury"); return; }
        for (pi = 0; pi < g_proc_count; pi++) {
            if (NStrEq(g_procs[pi].name, pname)) {
                if (g_procs[pi].is_function || g_procs[pi].param_count != 0) {
                    OutError("ON ERROR CALL wymaga PROCEDURE bez parametrow");
                    return;
                }
                g_error_handler_proc = pi;
                return;
            }
        }
        OutError("ON ERROR CALL: nieznana procedura");
        return;
    }

    // --- BEGIN / END (znaczniki struktury — ignoruj jako linie) ---
    if (NStrEq(line, "BEGIN") || NStrEq(line, "END")) return;
    if (NStartsWith(line, "RECORD")) return;
    if (NStartsWith(line, "WITH")) return;

    {
        char err[160];
        char tok[64];
        tok[0] = '\0';
        ParseIdent(line, tok, sizeof(tok));
        NStrCopy(err, "Nieznana instrukcja: ", sizeof(err));
        if (tok[0]) NStrAppend(err, tok, sizeof(err));
        else NStrAppend(err, line, sizeof(err));
        OutError(err);
    }
}

// ============================================================
// WYKONANIE ZAKRESU LINII
// ============================================================
static void ExecLines(uint32_t from, uint32_t to, uint32_t block_indent) {
    (void)block_indent;
    uint32_t i = from;
    while (i < to && i < g_line_count) {
        if (g_exit_flag || g_return_flag || g_continue_flag) break;
        uint32_t saved = g_cur_line;
        ExecLine(i, block_indent);
        SchedulerPoll();
        // ExecLine może zmienić g_cur_line (np. po IF lub FOR)
        if (g_cur_line != saved && g_cur_line > i)
            i = g_cur_line;
        else
            i++;
    }
}

static NyotaVal CallNamed(const char *name, const char *paren, const char **after_out) {
    NyotaVal none;
    ValClear(&none);
    if (after_out) *after_out = paren;

    NyotaProc *p = 0;
    uint32_t pi;
    for (pi = 0; pi < g_proc_count; pi++) {
        if (NStrEq(g_procs[pi].name, name)) { p = &g_procs[pi]; break; }
    }
    if (!p) {
        NyotaRecordDef *rd = FindRecordDef(name);
        if (rd) {
            const char *q;
            if (!paren || *paren != '(') {
                OutError("Konstruktor RECORD wymaga nawiasow");
                return none;
            }
            q = NTrim(paren + 1);
            if (*q != ')') {
                OutError("Konstruktor RECORD nie przyjmuje argumentow");
                return none;
            }
            if (after_out) *after_out = q + 1;
            return RecordCreateInstance(rd);
        }
        {
            char err[128];
            NStrCopy(err, "Nieznana procedura/funkcja/RECORD: ", sizeof(err));
            NStrAppend(err, name, sizeof(err));
            OutError(err);
        }
        return none;
    }
    if (!paren || *paren != '(') {
        OutError("Wymagane nawiasy przy wywolaniu");
        return none;
    }

    const char *q = paren + 1;
    int depth = 0, in_str = 0;
    const char *args_beg = q;
    while (*q) {
        if (*q == '"') in_str = !in_str;
        else if (!in_str && (*q == '(' || *q == '[')) depth++;
        else if (!in_str && (*q == ')' || *q == ']')) {
            if (*q == ')' && depth == 0) break;
            if (depth > 0) depth--;
        }
        q++;
    }
    if (*q != ')') {
        OutError("Brak zamykajacego nawiasu");
        return none;
    }
    if (after_out) *after_out = q + 1;

    char argbuf[512];
    uint32_t alen = (uint32_t)(q - args_beg);
    if (alen >= sizeof(argbuf)) alen = sizeof(argbuf) - 1;
    {
        uint32_t ai;
        for (ai = 0; ai < alen; ai++) argbuf[ai] = args_beg[ai];
        argbuf[alen] = '\0';
    }

    char args[8][MAX_STR_LEN];
    int nargs = 0;
    if (NTrim(argbuf)[0])
        nargs = SplitFunctionArgs(argbuf, args, 8);

    if ((uint32_t)nargs != p->param_count) {
        OutError("Nieprawidlowa liczba argumentow");
        return none;
    }

    NyotaVal argv[8];
    NyotaVar *ref_orig[8];
    {
        int ai;
        for (ai = 0; ai < nargs; ai++) {
            ValClear(&argv[ai]);
            ref_orig[ai] = 0;
            if (p->param_by_ref[ai]) {
                const char *it = NTrim(args[ai]);
                char vname[64];
                uint32_t vn = ParseIdent(it, vname, sizeof(vname));
                if (!vname[0] || *NTrim(it + vn)) {
                    OutError("Parametr VAR wymaga nazwy zmiennej");
                    return none;
                }
                ref_orig[ai] = FindVar(vname);
                if (!ref_orig[ai]) {
                    OutError("Zmienna niezadeklarowana (parametr VAR)");
                    return none;
                }
                argv[ai] = ref_orig[ai]->val;
            } else {
                argv[ai] = Eval(NTrim(args[ai]));
            }
        }
    }

    if (g_call_depth >= MAX_CALL_DEPTH) {
        OutError("Zbyt glebokie wywolanie");
        return none;
    }

    uint8_t saved_ret_flag = g_return_flag;
    NyotaVal saved_ret = g_return_val;
    uint32_t saved_line = g_cur_line;

    g_call_stack[g_call_depth] = g_cur_line;
    g_call_depth++;
    g_in_function[g_call_depth] = p->is_function;
    g_return_flag = 0;
    ValClear(&g_return_val);

    {
        uint32_t ai;
        for (ai = 0; ai < p->param_count; ai++) {
            NyotaVar *lv = CreateVar(p->params[ai]);
            if (!lv) {
                OutError("Za duzo zmiennych");
                break;
            }
            lv->val = argv[ai];
        }
    }

    {
        uint32_t def_indent = NIndent(g_lines[p->start_line]);
        uint32_t body_end = SkipBlock(p->body_line, def_indent);
        ExecLines(p->body_line, body_end, def_indent + NYOTA_INDENT);
    }

    NyotaVal result;
    ValClear(&result);
    if (p->is_function) {
        if (!g_return_flag) {
            OutError("FUNCTION bez RETURN");
        } else {
            result = g_return_val;
            if (result.type != TYPE_NONE) {
                if (p->return_type_known && p->return_type != result.type) {
                    OutError("FUNCTION zwraca rozne typy na roznych sciezkach/wywolaniach");
                    ValClear(&result);
                } else if (!p->return_type_known) {
                    p->return_type = result.type;
                    p->return_type_known = 1;
                }
            }
        }
    }

    {
        uint32_t ai;
        for (ai = 0; ai < p->param_count; ai++) {
            if (!p->param_by_ref[ai] || !ref_orig[ai]) continue;
            NyotaVar *lv = 0;
            uint32_t vi;
            for (vi = 0; vi < g_var_count; vi++) {
                if (g_vars[vi].scope == g_call_depth && NStrEq(g_vars[vi].name, p->params[ai])) {
                    lv = &g_vars[vi];
                    break;
                }
            }
            if (lv) ref_orig[ai]->val = lv->val;
        }
    }

    {
        uint32_t new_count = 0;
        uint32_t vi;
        for (vi = 0; vi < g_var_count; vi++) {
            if (g_vars[vi].scope < g_call_depth)
                g_vars[new_count++] = g_vars[vi];
        }
        g_var_count = new_count;
    }

    g_in_function[g_call_depth] = 0;
    if (g_call_depth > 0) g_call_depth--;
    g_return_flag = saved_ret_flag;
    g_return_val = saved_ret;
    g_cur_line = saved_line;
    return result;
}

// ============================================================
// WYSZUKANIE I WYKONANIE BLOKU BEGIN..END
// ============================================================
static int NLineBlankOrComment(const char *raw) {
    const char *t = NTrim(raw);
    return !*t || *t == '#';
}

static void RunProgram(void) {
    uint32_t begin_line = 0xFFFFFFFF;
    uint32_t end_line = 0xFFFFFFFF;
    uint32_t first_end = 0xFFFFFFFF;
    uint32_t begin_count = 0;
    uint32_t end_count = 0;

    for (uint32_t i = 0; i < g_line_count; i++) {
        const char *l = NTrim(g_lines[i]);
        if (NStrEq(l, "BEGIN")) {
            begin_count++;
            if (begin_line == 0xFFFFFFFF) begin_line = i;
        } else if (NStrEq(l, "END")) {
            end_count++;
            if (first_end == 0xFFFFFFFF) first_end = i;
            if (begin_line != 0xFFFFFFFF && end_line == 0xFFFFFFFF) end_line = i;
        }
    }

    if (begin_count == 0) {
        g_cur_line = 0;
        for (uint32_t j = 0; j < g_line_count; j++) {
            if (!NLineBlankOrComment(g_lines[j])) { g_cur_line = j; break; }
        }
        OutError("Brak BEGIN");
        return;
    }
    if (end_count == 0) {
        g_cur_line = begin_line;
        OutError("Brak END");
        return;
    }
    if (first_end < begin_line) {
        g_cur_line = first_end;
        OutError("END przed BEGIN");
        return;
    }
    if (begin_count > 1) {
        g_cur_line = begin_line;
        OutError("Wiecej niz jedno BEGIN");
        return;
    }
    if (end_count > 1) {
        g_cur_line = end_line;
        OutError("Wiecej niz jedno END");
        return;
    }

    uint32_t i = 0;
    while (i < begin_line) {
        g_cur_line = i;
        if (NLineBlankOrComment(g_lines[i])) { i++; continue; }
        const char *l = NTrim(g_lines[i]);
        if (NIndent(g_lines[i]) != 0) {
            OutError("Instrukcja poza BEGIN ... END");
            return;
        }
        if (NStartsWith(l, "FUNCTION") || NStartsWith(l, "PROCEDURE") || NStartsWith(l, "RECORD")) {
            i = SkipBlock(i + 1, 0);
            continue;
        }
        if (NStartsWith(l, "CONST") || NStartsWith(l, "IMPORT")) {
            i++;
            continue;
        }
        OutError("Instrukcja poza BEGIN ... END");
        return;
    }

    for (i = end_line + 1; i < g_line_count; i++) {
        g_cur_line = i;
        if (NLineBlankOrComment(g_lines[i])) continue;
        OutError("Instrukcja po END");
        return;
    }

    for (i = 0; i < begin_line; i++) {
        const char *l = NTrim(g_lines[i]);
        if (NStartsWith(l, "CONST")) ExecLine(i, 0);
    }

    ExecLines(begin_line + 1, end_line, 0);
}

// ============================================================
// RYSOWANIE EKRANU STARTOWEGO INTERPRETERA
// ============================================================
static void DrawHeader(const char *filename) {
    HostRect(0, 0, SCREEN_W, SCREEN_H, 10, 10, 20);
    HostRect(0, 0, SCREEN_W, 36, 20, 20, 40);
    HostText(OUT_MARGIN, 8, "Nyota Interpreter v0.5 | AyoOS", 100, 180, 255, 2);
    HostText(OUT_MARGIN + 700, 8, filename, 160, 160, 200, 2);
    HostRect(0, 36, SCREEN_W, 2, 50, 50, 100);
    g_out_x = OUT_MARGIN;
    g_out_y = 44;
}

#ifdef NYOTA_EMBEDDED
static void NyotaEmbedReset(void) {
    {
        uint32_t i;
        for (i = 0; i < g_sprite_count; i++)
            if (g_sprites[i].host_handle >= 0)
                HostSpriteFree(g_sprites[i].host_handle);
    }
    g_sprite_count = 0;
    g_var_count = 0;
    g_proc_count = 0;
    g_record_count = 0;
    g_with_var = 0;
    g_call_depth = 0;
    g_in_function[0] = 0;
    g_exit_flag = 0;
    g_continue_flag = 0;
    g_loop_depth = 0;
    g_return_flag = 0;
    g_error_handler_proc = 0xFFFFFFFF;
    g_err_code = 0;
    g_in_error_handler = 0;
    g_list_pool_used = 0;
    g_table_count = 0;
    g_button_count = 0;
    g_ui_control_count = 0;
    g_window_count = 0;
    g_is_graphics = 0;
    g_graph_w = g_graph_h = 0;
    g_active_screen = 0;
    {
        uint32_t si;
        for (si = 0; si < MAX_SCREENS; si++) g_screen_exists[si] = 0;
    }
    g_scheduler_running = 0;
    {
        int i;
        for (i = 0; i < MAX_EVERY_EVENTS; i++) g_every[i].active = 0;
        for (i = 0; i < MAX_NN; i++) g_nn[i].active = 0;
        for (i = 0; i < MAX_STATES; i++) g_states_active[i] = 0;
    }
}

void NyotaEmbedRun(const char *src, void (*emit)(char c)) {
    g_ny_emit = emit;
    g_source_size = 0;
    if (!src) src = "";
    while (src[g_source_size] && g_source_size + 1 < MAX_SOURCE) {
        g_source[g_source_size] = (uint8_t)src[g_source_size];
        g_source_size++;
    }
    g_source[g_source_size] = '\0';

    if (g_host && g_host->ticks_100hz)
        g_rng = (uint32_t)HostTicks() ^ 0xDEADBEEF;

    NyotaEmbedReset();
    ParseSourceToLines();
    if (!ExpandImports()) {
        g_ny_emit = 0;
        return;
    }
    if (!ValidateSourceLayout()) {
        g_ny_emit = 0;
        return;
    }
    if (!ScanRecords()) {
        g_ny_emit = 0;
        return;
    }
    ScanProcedures();
    NyotaEmbedReset();
    if (!ScanRecords()) {
        g_ny_emit = 0;
        return;
    }
    ScanProcedures();
    if (!ValidateProcedureContracts()) {
        g_ny_emit = 0;
        return;
    }
    g_ny_running = 1;
    RunProgram();
    g_ny_emit = 0;
}
#endif

#ifndef NYOTA_EMBEDDED
// ============================================================
// MAIN
// ============================================================
void _start(AyoAPI *api) {
    static NyotaHost ayo_host;
    g_api = api;
    ayo_host.gfx_rect = api->DrawRect;
    ayo_host.gfx_text = api->DrawText;
    ayo_host.gfx_clear = api->ClearScreen;
    ayo_host.gfx_mode = api->SetResolution;
    ayo_host.ticks_100hz = api->GetTicks;
    ayo_host.unix_time = api->GetUnixTime;
    ayo_host.wait_key = api->WaitForKey;
    ayo_host.key_mods = api->GetKeyModifiers;
    NyotaSetHost(&ayo_host);
    HostGfxMode(SCREEN_W, SCREEN_H);

    // Wczytaj ścieżkę pliku z _nyotarun
    char nyo_path[256]; nyo_path[0] = '\0';
    uint32_t path_size = 0;
    uint8_t path_buf[256];
    if (g_api->ReadFile(RUN_FILE_PATH, path_buf, sizeof(path_buf) - 1, &path_size) == 0
        && path_size > 0) {
        path_buf[path_size] = '\0';
        // Usuń białe znaki
        uint32_t i = 0;
        while (path_buf[i] && path_buf[i] != '\n' && path_buf[i] != '\r'
               && i < sizeof(nyo_path) - 1) {
            nyo_path[i] = (char)path_buf[i]; i++;
        }
        nyo_path[i] = '\0';
    }

    if (!nyo_path[0]) {
        HostRect(0, 0, SCREEN_W, SCREEN_H, 10, 10, 20);
        HostText(OUT_MARGIN, 40, "NYOTA: Brak pliku do uruchomienia.", 255, 80, 80, 2);
        HostText(OUT_MARGIN, 80, "Uzyj F9 w AyoEdit lub zapisz sciezke do A:/System/_nyotarun", 180, 180, 180, 2);
        HostWaitKey();
        g_api->Exit();
        return;
    }

    DrawHeader(nyo_path);

    // Zainicjalizuj generator liczb losowych
    g_rng = (uint32_t)HostTicks() ^ 0xDEADBEEF;

    // Wczytaj źródło .nyo
    g_source_size = 0;
    if (g_api->ReadFile(nyo_path, g_source, MAX_SOURCE - 1, &g_source_size) != 0
        || g_source_size == 0) {
        OutError("Nie mozna odczytac pliku .nyo");
        HostText(OUT_MARGIN, g_out_y + 20, nyo_path, 200, 200, 100, 2);
        HostWaitKey();
        g_api->Exit();
        return;
    }
    g_source[g_source_size] = '\0';

    // Parsuj linie i rozwiń IMPORT
    ParseSourceToLines();
    if (!ExpandImports()) {
        HostWaitKey();
        g_api->Exit();
        return;
    }
    if (!ValidateSourceLayout()) {
        HostWaitKey();
        g_api->Exit();
        return;
    }

    // Pre-scan deklaracji
    if (!ScanRecords()) {
        HostWaitKey();
        g_api->Exit();
        return;
    }
    ScanProcedures();

    // Inicjalizacja stanu
    g_var_count = 0;
    g_proc_count = 0;
    g_record_count = 0;
    g_with_var = 0;
    g_call_depth = 0;
    g_in_function[0] = 0;
    g_exit_flag = 0;
    g_continue_flag = 0;
    g_loop_depth = 0;
    g_return_flag = 0;
    g_error_handler_proc = 0xFFFFFFFF;
    g_err_code = 0;
    g_in_error_handler = 0;
    g_list_pool_used = 0;
    g_table_count = 0;
    g_button_count = 0;
    g_is_graphics = 0;
    g_graph_w = g_graph_h = 0;
    g_active_screen = 0;
    for (int si = 0; si < MAX_SCREENS; si++) g_screen_exists[si] = 0;
    g_scheduler_running = 0;
    for (int i = 0; i < MAX_EVERY_EVENTS; i++) g_every[i].active = 0;
    for (int i = 0; i < MAX_NN; i++) g_nn[i].active = 0;
    for (int i = 0; i < MAX_STATES; i++) g_states_active[i] = 0;

    if (!ScanRecords()) {
        HostWaitKey();
        g_api->Exit();
        return;
    }
    ScanProcedures();  // po raz drugi żeby procedury były w g_procs
    if (!ValidateProcedureContracts()) {
        HostWaitKey();
        g_api->Exit();
        return;
    }
    g_ny_running = 1;

    // Wykonaj program
    RunProgram();

    // Koniec
    HostRect(0, g_out_y, SCREEN_W, OUT_FONT_H, 10, 10, 20);
    HostText(OUT_MARGIN, g_out_y, "--- Program zakonczony. Nacisnij dowolny klawisz. ---",
                    100, 180, 100, 2);
    HostWaitKey();
    g_api->Exit();
}
#endif /* !NYOTA_EMBEDDED */
