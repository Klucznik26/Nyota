from pathlib import Path
import struct

def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"{path}: expected exactly one match, got {n}\nANCHOR:\n{old[:500]}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")

replace_once(
    "src/nyota_host.h",
    """    void (*gfx_mode)(uint32_t w, uint32_t h);
} NyotaHost;
""",
    """    void (*gfx_mode)(uint32_t w, uint32_t h);

    /* Sprite backend. Language-level SPRITE is named state in Nyota;
     * host owns only the loaded image resource and drawing. */
    int32_t (*gfx_sprite_load)(const char *path);
    void (*gfx_sprite_free)(int32_t handle);
    void (*gfx_sprite_draw)(int32_t handle, int32_t x, int32_t y,
                            uint32_t w, uint32_t h);
} NyotaHost;
""",
)

replace_once(
    "src/nyota.c",
    """static void HostGfxMode(uint32_t w, uint32_t h) {
    if (g_host && g_host->gfx_mode) g_host->gfx_mode(w, h);
}

static uint64_t HostTicks(void) {
""",
    """static void HostGfxMode(uint32_t w, uint32_t h) {
    if (g_host && g_host->gfx_mode) g_host->gfx_mode(w, h);
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

static uint64_t HostTicks(void) {
""",
)

replace_once(
    "src/nyota.c",
    """#define MAX_BUTTONS     64      // max nazwanych kontrolek BUTTON
#define NYOTA_INDENT    4       // jeden poziom bloku = dokładnie 4 spacje
""",
    """#define MAX_BUTTONS     64      // max nazwanych kontrolek BUTTON
#define MAX_SPRITES     64      // max nazwanych obiektow SPRITE
#define NYOTA_INDENT    4       // jeden poziom bloku = dokładnie 4 spacje
""",
)

replace_once(
    "src/nyota.c",
    """typedef struct {
    char name[64];
    int32_t x, y, w, h;
    char text[MAX_STR_LEN];
    char font[64];
    uint32_t font_size;
    uint8_t text_r, text_g, text_b;
    uint8_t bg_r, bg_g, bg_b;
    uint8_t prev_down;
} NyotaButton;

// ============================================================
// STAN GLOBALNY INTERPRETERA
""",
    """typedef struct {
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
} NyotaSprite;

// ============================================================
// STAN GLOBALNY INTERPRETERA
""",
)

replace_once(
    "src/nyota.c",
    """static NyotaButton g_buttons[MAX_BUTTONS];
static uint32_t    g_button_count = 0;

// TinyML: Mikro-Sieć Neuronowa (Perceptron bez FPU)
""",
    """static NyotaButton g_buttons[MAX_BUTTONS];
static uint32_t    g_button_count = 0;
static NyotaSprite g_sprites[MAX_SPRITES];
static uint32_t    g_sprite_count = 0;

// TinyML: Mikro-Sieć Neuronowa (Perceptron bez FPU)
""",
)

replace_once(
    "src/nyota.c",
    """static NyotaVal ParsePrimary(const char **pp);
static NyotaButton *FindButton(const char *name);
static int ButtonPollClicked(NyotaButton *b);

static NyotaVal ValArith""",
    """static NyotaVal ParsePrimary(const char **pp);
static NyotaButton *FindButton(const char *name);
static int ButtonPollClicked(NyotaButton *b);
static NyotaSprite *FindSprite(const char *name);
static int SpriteArgName(const char *arg, char *out, uint32_t out_size);
static int SpriteHit(const NyotaSprite *a, const NyotaSprite *b);

static NyotaVal ValArith""",
)

sprite_expr = r'''    if (NStrEqN(expr, "SPRITE_X(", 9) || NStrEqN(expr, "SPRITE_Y(", 9) ||
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
'''

replace_once(
    "src/nyota.c",
    """    if (NStrEqN(expr, "BUTTON_CLICKED(", 15)) {
""",
    sprite_expr + """    if (NStrEqN(expr, "BUTTON_CLICKED(", 15)) {
""",
)

sprite_helpers = r'''
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
    if (!HostSpriteDraw(s->host_handle, s->x, s->y,
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

'''

replace_once(
    "src/nyota.c",
    """// ============================================================
// POMIŃ BLOK (skocz za blok wcięty o więcej niż cur_indent)
""",
    sprite_helpers + """// ============================================================
// POMIŃ BLOK (skocz za blok wcięty o więcej niż cur_indent)
""",
)

sprite_commands = r'''    // --- SPRITE_DRAW nazwa ---
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
        return;
    }

'''

replace_once(
    "src/nyota.c",
    """    // --- VAR ---
""",
    sprite_commands + """    // --- VAR ---
""",
)

replace_once(
    "src/nyota.c",
    """static void NyotaEmbedReset(void) {
    g_var_count = 0;
""",
    """static void NyotaEmbedReset(void) {
    {
        uint32_t i;
        for (i = 0; i < g_sprite_count; i++)
            if (g_sprites[i].host_handle >= 0)
                HostSpriteFree(g_sprites[i].host_handle);
    }
    g_sprite_count = 0;
    g_var_count = 0;
""",
)

replace_once(
    "host/posix/host.c",
    """static int g_input_pos;

static void posix_emit(char c) {
""",
    """static int g_input_pos;

#define HOST_MAX_SPRITES 64
static SDL_Texture *g_sprite_tex[HOST_MAX_SPRITES];

static void posix_emit(char c) {
""",
)

sprite_host = r'''
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

'''

replace_once(
    "host/posix/host.c",
    """static uint8_t key_to_scancode(SDL_Keycode k) {
""",
    sprite_host + """static uint8_t key_to_scancode(SDL_Keycode k) {
""",
)

replace_once(
    "host/posix/host.c",
    """static void host_exit(void) {
    if (g_gfx) {
        SDL_DestroyRenderer(g_ren);
""",
    """static void host_exit(void) {
    if (g_gfx) {
        int i;
        for (i = 0; i < HOST_MAX_SPRITES; i++) {
            if (g_sprite_tex[i]) {
                SDL_DestroyTexture(g_sprite_tex[i]);
                g_sprite_tex[i] = 0;
            }
        }
        SDL_DestroyRenderer(g_ren);
""",
)

replace_once(
    "host/posix/host.c",
    """    g_nyhost.gfx_text = host_text;
    g_nyhost.gfx_mode = host_setres;
    NyotaSetHost(&g_nyhost);
""",
    """    g_nyhost.gfx_text = host_text;
    g_nyhost.gfx_mode = host_setres;
    g_nyhost.gfx_sprite_load = host_sprite_load;
    g_nyhost.gfx_sprite_free = host_sprite_free;
    g_nyhost.gfx_sprite_draw = host_sprite_draw;
    NyotaSetHost(&g_nyhost);
""",
)

tests = {
"tests/graph_sprite_basic.nyo": '''# Oczekiwane: OK
BEGIN
GRAPH 1
SPRITE gracz, "tests/assets/sprite_test.bmp", 10, 20, 16, 18
IF SPRITE_X(gracz) <> 10:
    PRNIT "x"
IF SPRITE_Y(gracz) <> 20:
    PRNIT "y"
IF SPRITE_W(gracz) <> 16:
    PRNIT "w"
IF SPRITE_H(gracz) <> 18:
    PRNIT "h"
SPRITE_MOVE gracz, 5, -2
IF SPRITE_X(gracz) <> 15:
    PRNIT "move-x"
IF SPRITE_Y(gracz) <> 18:
    PRNIT "move-y"
SPRITE_POS gracz, 100, 110
SPRITE_DRAW gracz
SPRITE_HIDE gracz
IF SPRITE_VISIBLE(gracz):
    PRNIT "hide"
SPRITE_DRAW gracz
SPRITE_SHOW gracz
IF NOT SPRITE_VISIBLE(gracz):
    PRNIT "show"
SPRITE_DRAW gracz
SPRITE_DELETE gracz
END
''',
"tests/graph_sprite_hit.nyo": '''# Oczekiwane: OK
BEGIN
GRAPH 1
SPRITE a, "tests/assets/sprite_test.bmp", 0, 0, 10, 10
SPRITE b, "tests/assets/sprite_test.bmp", 9, 9, 10, 10
IF NOT SPRITE_HIT(a, b):
    PRNIT "hit"
SPRITE_POS b, 10, 10
IF SPRITE_HIT(a, b):
    PRNIT "edge"
SPRITE_POS b, 50, 50
IF SPRITE_HIT(a, b):
    PRNIT "far"
SPRITE_HIDE a
SPRITE_POS b, 5, 5
IF NOT SPRITE_HIT(a, b):
    PRNIT "visibility-must-not-disable-hit"
END
''',
"tests/graph_sprite_missing.nyo": '''# Oczekiwane: BLAD
BEGIN
GRAPH 1
SPRITE brak, "tests/assets/does_not_exist.bmp", 0, 0, 8, 8
END
''',
"tests/graph_sprite_bad_size.nyo": '''# Oczekiwane: BLAD
BEGIN
GRAPH 1
SPRITE zly, "tests/assets/sprite_test.bmp", 0, 0, 0, 8
END
''',
"tests/graph_sprite_unknown.nyo": '''# Oczekiwane: BLAD
BEGIN
GRAPH 1
SPRITE_DRAW nie_ma
END
''',
"tests/sprite_without_graph.nyo": '''# Oczekiwane: BLAD
BEGIN
SPRITE gracz, "tests/assets/sprite_test.bmp", 0, 0, 8, 8
END
''',
}

for path, content in tests.items():
    Path(path).write_text(content, encoding="utf-8")

asset = Path("tests/assets")
asset.mkdir(parents=True, exist_ok=True)
w, h = 2, 2
row_size = (w * 3 + 3) & ~3
pixel_size = row_size * h
file_size = 54 + pixel_size
header = bytearray()
header += b"BM"
header += struct.pack("<IHHI", file_size, 0, 0, 54)
header += struct.pack("<IIIHHIIIIII", 40, w, h, 1, 24, 0, pixel_size, 2835, 2835, 0, 0)
row0 = bytes([0, 0, 255, 0, 255, 0]) + b"\x00" * (row_size - 6)
row1 = bytes([255, 0, 0, 255, 255, 255]) + b"\x00" * (row_size - 6)
asset.joinpath("sprite_test.bmp").write_bytes(bytes(header) + row0 + row1)

print("SPRITE upgrade applied")
