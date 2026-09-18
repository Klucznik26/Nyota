from pathlib import Path

def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"{path}: expected exactly one match, got {n}\nANCHOR:\n{old[:600]}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")

# Host contract: drawing one frame from a horizontal strip.
replace_once(
    "src/nyota_host.h",
    """    void (*gfx_sprite_draw)(int32_t handle, int32_t x, int32_t y,
                            uint32_t w, uint32_t h);
} NyotaHost;
""",
    """    void (*gfx_sprite_draw)(int32_t handle, int32_t x, int32_t y,
                            uint32_t w, uint32_t h);
    void (*gfx_sprite_draw_frame)(int32_t handle, uint32_t frame,
                                  uint32_t frame_count,
                                  int32_t x, int32_t y,
                                  uint32_t w, uint32_t h);
} NyotaHost;
""",
)

# Core wrapper.
replace_once(
    "src/nyota.c",
    """static int HostSpriteDraw(int32_t handle, int32_t x, int32_t y,
                          uint32_t w, uint32_t h) {
    if (!g_host || !g_host->gfx_sprite_draw) return 0;
    g_host->gfx_sprite_draw(handle, x, y, w, h);
    return 1;
}

static uint64_t HostTicks(void) {
""",
    """static int HostSpriteDraw(int32_t handle, int32_t x, int32_t y,
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
""",
)

# Animation state.
replace_once(
    "src/nyota.c",
    """typedef struct {
    char name[64];
    char source[MAX_STR_LEN];
    int32_t x, y, w, h;
    int32_t host_handle;
    uint8_t visible;
} NyotaSprite;
""",
    """typedef struct {
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
""",
)

# Forward declaration for expression queries.
replace_once(
    "src/nyota.c",
    """static NyotaSprite *FindSprite(const char *name);
static int SpriteArgName(const char *arg, char *out, uint32_t out_size);
static int SpriteHit(const NyotaSprite *a, const NyotaSprite *b);

static NyotaVal ValArith""",
    """static NyotaSprite *FindSprite(const char *name);
static int SpriteArgName(const char *arg, char *out, uint32_t out_size);
static int SpriteHit(const NyotaSprite *a, const NyotaSprite *b);
static void SpriteUpdateAnimation(NyotaSprite *s);

static NyotaVal ValArith""",
)

# Expression functions: current frame and playback state.
sprite_expr = r'''    if (NStrEqN(expr, "SPRITE_FRAME(", 13) ||
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
'''

replace_once(
    "src/nyota.c",
    """    if (NStrEqN(expr, "SPRITE_HIT(", 11)) {
""",
    sprite_expr + """    if (NStrEqN(expr, "SPRITE_HIT(", 11)) {
""",
)

# Initialize animation defaults for new sprites.
replace_once(
    "src/nyota.c",
    """    s->w = s->h = 1;
    s->host_handle = -1;
    s->visible = 1;
    NStrCopy(s->name, name, sizeof(s->name));
""",
    """    s->w = s->h = 1;
    s->host_handle = -1;
    s->visible = 1;
    s->frame_count = 1;
    s->frame = 0;
    s->frame_ms = 100;
    s->playing = 0;
    s->last_frame_tick = 0;
    NStrCopy(s->name, name, sizeof(s->name));
""",
)

# Time-driven animation and frame-aware rendering.
replace_once(
    "src/nyota.c",
    """static int RenderSprite(NyotaSprite *s) {
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
""",
    """static void SpriteUpdateAnimation(NyotaSprite *s) {
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
""",
)

# Animation commands before SPRITE_DRAW.
sprite_commands = r'''    // --- SPRITE_ANIM nazwa, liczba_klatek, ms_na_klatke ---
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

'''

replace_once(
    "src/nyota.c",
    """    // --- SPRITE_DRAW nazwa ---
""",
    sprite_commands + """    // --- SPRITE_DRAW nazwa ---
""",
)

# Redefining a sprite resets animation to a static frame.
replace_once(
    "src/nyota.c",
    """        s->x = x; s->y = y; s->w = w; s->h = h;
        s->host_handle = handle;
        s->visible = 1;
        return;
""",
    """        s->x = x; s->y = y; s->w = w; s->h = h;
        s->host_handle = handle;
        s->visible = 1;
        s->frame_count = 1;
        s->frame = 0;
        s->frame_ms = 100;
        s->playing = 0;
        s->last_frame_tick = 0;
        return;
""",
)

# POSIX/SDL2: crop equal horizontal frames from the texture.
replace_once(
    "host/posix/host.c",
    """static void host_sprite_draw(int32_t handle, int32_t x, int32_t y,
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

static uint8_t key_to_scancode(SDL_Keycode k) {
""",
    """static void host_sprite_draw(int32_t handle, int32_t x, int32_t y,
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
""",
)

replace_once(
    "host/posix/host.c",
    """    g_nyhost.gfx_sprite_free = host_sprite_free;
    g_nyhost.gfx_sprite_draw = host_sprite_draw;
    NyotaSetHost(&g_nyhost);
""",
    """    g_nyhost.gfx_sprite_free = host_sprite_free;
    g_nyhost.gfx_sprite_draw = host_sprite_draw;
    g_nyhost.gfx_sprite_draw_frame = host_sprite_draw_frame;
    NyotaSetHost(&g_nyhost);
""",
)

# Regression tests. Current 2x2 BMP works as a two-frame horizontal strip.
tests = {
"tests/graph_sprite_anim.nyo": '''# Oczekiwane: OK
BEGIN
GRAPH 1
SPRITE gracz, "tests/assets/sprite_test.bmp", 10, 10, 24, 24
SPRITE_ANIM gracz, 2, 20
IF NOT SPRITE_PLAYING(gracz):
    PRNIT "play"
SPRITE_DRAW gracz
SPRITE_STOP gracz
IF SPRITE_PLAYING(gracz):
    PRNIT "stop"
SPRITE_FRAME gracz, 1
IF SPRITE_FRAME(gracz) <> 1:
    PRNIT "frame"
SPRITE_DRAW gracz
SPRITE_PLAY gracz
DELAY 30
SPRITE_DRAW gracz
END
''',
"tests/graph_sprite_anim_bad_count.nyo": '''# Oczekiwane: BLAD
BEGIN
GRAPH 1
SPRITE gracz, "tests/assets/sprite_test.bmp", 0, 0, 16, 16
SPRITE_ANIM gracz, 0, 100
END
''',
"tests/graph_sprite_anim_bad_frame.nyo": '''# Oczekiwane: BLAD
BEGIN
GRAPH 1
SPRITE gracz, "tests/assets/sprite_test.bmp", 0, 0, 16, 16
SPRITE_ANIM gracz, 2, 100
SPRITE_FRAME gracz, 2
END
''',
"tests/graph_sprite_anim_bad_time.nyo": '''# Oczekiwane: BLAD
BEGIN
GRAPH 1
SPRITE gracz, "tests/assets/sprite_test.bmp", 0, 0, 16, 16
SPRITE_ANIM gracz, 2, 0
END
''',
}
for path, content in tests.items():
    Path(path).write_text(content, encoding="utf-8")

print("SPRITE animation upgrade applied")
