from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"{path}: expected exactly one match, got {n}\nANCHOR:\n{old[:240]}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# ---------------- src/nyota_host.h ----------------
replace_once(
    "src/nyota_host.h",
    "    uint8_t (*wait_key)(void);\n    uint8_t (*key_mods)(void);\n\n    void (*gfx_clear)(uint8_t r, uint8_t g, uint8_t b);\n",
    "    uint8_t (*wait_key)(void);\n    uint8_t (*key_mods)(void);\n    /* Zwraca maskę przycisków wskaźnika: bit 0 = lewy. */\n    uint8_t (*pointer_state)(int32_t *x, int32_t *y);\n\n    void (*gfx_clear)(uint8_t r, uint8_t g, uint8_t b);\n",
)


# ---------------- host/posix/host.c ----------------
replace_once(
    "host/posix/host.c",
    "static uint8_t host_mods(void) {\n    return g_shift ? 1 : 0;\n}\n\nstatic void host_exit(void) {\n",
    "static uint8_t host_mods(void) {\n    return g_shift ? 1 : 0;\n}\n\nstatic uint8_t host_pointer_state(int32_t *x, int32_t *y) {\n    int mx = 0, my = 0;\n    uint32_t state;\n    if (!g_gfx) {\n        if (x) *x = 0;\n        if (y) *y = 0;\n        return 0;\n    }\n    host_pump();\n    state = SDL_GetMouseState(&mx, &my);\n    if (x) *x = (int32_t)mx;\n    if (y) *y = (int32_t)my;\n    return (state & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1U : 0U;\n}\n\nstatic void host_exit(void) {\n",
)
replace_once(
    "host/posix/host.c",
    "    g_nyhost.wait_key = host_waitkey;\n    g_nyhost.key_mods = host_mods;\n    g_nyhost.gfx_clear = host_clear;\n",
    "    g_nyhost.wait_key = host_waitkey;\n    g_nyhost.key_mods = host_mods;\n    g_nyhost.pointer_state = host_pointer_state;\n    g_nyhost.gfx_clear = host_clear;\n",
)


# ---------------- src/nyota.c ----------------
replace_once(
    "src/nyota.c",
    "static uint8_t HostKeyMods(void) {\n    if (g_host && g_host->key_mods) return g_host->key_mods();\n    return 0;\n}\n\n// ============================================================\n// KONFIGURACJA\n",
    "static uint8_t HostKeyMods(void) {\n    if (g_host && g_host->key_mods) return g_host->key_mods();\n    return 0;\n}\n\nstatic uint8_t HostPointerState(int32_t *x, int32_t *y) {\n    if (g_host && g_host->pointer_state) return g_host->pointer_state(x, y);\n    if (x) *x = 0;\n    if (y) *y = 0;\n    return 0;\n}\n\n// ============================================================\n// KONFIGURACJA\n",
)
replace_once(
    "src/nyota.c",
    "#define MAX_TABLES      32      // max nazwanych kontrolek TABLE\n#define MAX_TABLE_COLS  16      // max kolumn jednej TABLE\n#define NYOTA_INDENT    4       // jeden poziom bloku = dokładnie 4 spacje\n",
    "#define MAX_TABLES      32      // max nazwanych kontrolek TABLE\n#define MAX_TABLE_COLS  16      // max kolumn jednej TABLE\n#define MAX_BUTTONS     64      // max nazwanych kontrolek BUTTON\n#define NYOTA_INDENT    4       // jeden poziom bloku = dokładnie 4 spacje\n",
)
replace_once(
    "src/nyota.c",
    "} NyotaTable;\n\n// ============================================================\n// STAN GLOBALNY INTERPRETERA\n",
    "} NyotaTable;\n\n// BUTTON jest nazwaną kontrolką GUI, a nie typem zmiennej Nyoty.\ntypedef struct {\n    char name[64];\n    int32_t x, y, w, h;\n    char text[MAX_STR_LEN];\n    char font[64];\n    uint32_t font_size;\n    uint8_t text_r, text_g, text_b;\n    uint8_t bg_r, bg_g, bg_b;\n    uint8_t prev_down;\n} NyotaButton;\n\n// ============================================================\n// STAN GLOBALNY INTERPRETERA\n",
)
replace_once(
    "src/nyota.c",
    "static NyotaTable g_tables[MAX_TABLES];\nstatic uint32_t   g_table_count = 0;\n\n// TinyML: Mikro-Sieć Neuronowa (Perceptron bez FPU)\n",
    "static NyotaTable g_tables[MAX_TABLES];\nstatic uint32_t   g_table_count = 0;\nstatic NyotaButton g_buttons[MAX_BUTTONS];\nstatic uint32_t    g_button_count = 0;\n\n// TinyML: Mikro-Sieć Neuronowa (Perceptron bez FPU)\n",
)
replace_once(
    "src/nyota.c",
    "static NyotaVal CallNamed(const char *name, const char *paren, const char **after_out);\nstatic NyotaVal ParseOr(const char **pp);\nstatic NyotaVal ParsePrimary(const char **pp);\n",
    "static NyotaVal CallNamed(const char *name, const char *paren, const char **after_out);\nstatic NyotaVal ParseOr(const char **pp);\nstatic NyotaVal ParsePrimary(const char **pp);\nstatic NyotaButton *FindButton(const char *name);\nstatic int ButtonPollClicked(NyotaButton *b);\n",
)

# BUTTON_CLICKED(name) / BUTTON_CLICKED("name")
replace_once(
    "src/nyota.c",
    "    if (NStrEqN(expr, \"LEN(\", 4)) {\n",
    "    if (NStrEqN(expr, \"BUTTON_CLICKED(\", 15)) {\n        char args[2][MAX_STR_LEN];\n        char bname[64];\n        int n = SplitFunctionArgs(expr + 15, args, 2);\n        NyotaButton *b;\n        int clicked;\n        if (n != 1) {\n            OutError(\"BUTTON_CLICKED() wymaga jednej nazwy BUTTON\");\n            ValClear(&result);\n            *pp = call_open ? MatchParen(call_open) : expr;\n            return result;\n        }\n        {\n            const char *a = NTrim(args[0]);\n            if (*a == '\"') {\n                NyotaVal nv = Eval(a);\n                if (nv.type != TYPE_STR || !nv.s[0]) {\n                    OutError(\"BUTTON_CLICKED() wymaga nazwy BUTTON\");\n                    ValClear(&result);\n                    *pp = call_open ? MatchParen(call_open) : expr;\n                    return result;\n                }\n                NStrCopy(bname, nv.s, sizeof(bname));\n            } else {\n                uint32_t bn = ParseIdent(a, bname, sizeof(bname));\n                if (!bname[0] || *NTrim(a + bn)) {\n                    OutError(\"BUTTON_CLICKED() wymaga identyfikatora albo STRING\");\n                    ValClear(&result);\n                    *pp = call_open ? MatchParen(call_open) : expr;\n                    return result;\n                }\n            }\n        }\n        b = FindButton(bname);\n        if (!b) {\n            OutError(\"BUTTON_CLICKED: nieznany BUTTON\");\n            ValClear(&result);\n            *pp = call_open ? MatchParen(call_open) : expr;\n            return result;\n        }\n        clicked = ButtonPollClicked(b);\n        if (clicked < 0) {\n            ValClear(&result);\n            *pp = call_open ? MatchParen(call_open) : expr;\n            return result;\n        }\n        ValFromBool(&result, clicked);\n        *pp = call_open ? MatchParen(call_open) : expr;\n        return result;\n    }\n    if (NStrEqN(expr, \"LEN(\", 4)) {\n",
)

# Helpers inserted after TABLE rendering, when DrawLine and TableClipText are already available.
replace_once(
    "src/nyota.c",
    "    return 1;\n}\n\n// ============================================================\n// POMIŃ BLOK (skocz za blok wcięty o więcej niż cur_indent)\n",
    "    return 1;\n}\n\n// ============================================================\n// BUTTON — nazwana kontrolka GUI\n// ============================================================\nstatic NyotaButton *FindButton(const char *name) {\n    uint32_t i;\n    for (i = 0; i < g_button_count; i++)\n        if (NStrEq(g_buttons[i].name, name)) return &g_buttons[i];\n    return 0;\n}\n\nstatic NyotaButton *GetOrCreateButton(const char *name) {\n    NyotaButton *b = FindButton(name);\n    if (b) return b;\n    if (g_button_count >= MAX_BUTTONS) {\n        OutError(\"Za duzo kontrolek BUTTON\");\n        return 0;\n    }\n    b = &g_buttons[g_button_count++];\n    NStrCopy(b->name, name, sizeof(b->name));\n    b->prev_down = 0;\n    return b;\n}\n\nstatic int ButtonEvalInt(const char *expr, int32_t *out) {\n    NyotaVal v = Eval(expr);\n    if (v.type != TYPE_INT) {\n        OutError(\"BUTTON: parametr liczbowy wymaga INTEGER\");\n        return 0;\n    }\n    *out = v.i;\n    return 1;\n}\n\nstatic void RenderButton(NyotaButton *b) {\n    uint32_t scale, text_w, text_h;\n    int32_t tx, ty;\n    char clipped[MAX_STR_LEN];\n    if (!b || !g_is_graphics) return;\n\n    HostRect((uint32_t)b->x, (uint32_t)b->y, (uint32_t)b->w, (uint32_t)b->h,\n             b->bg_r, b->bg_g, b->bg_b);\n    DrawLine(b->x, b->y, b->x + b->w - 1, b->y, 190, 190, 190);\n    DrawLine(b->x, b->y, b->x, b->y + b->h - 1, 190, 190, 190);\n    DrawLine(b->x, b->y + b->h - 1, b->x + b->w - 1, b->y + b->h - 1, 55, 55, 55);\n    DrawLine(b->x + b->w - 1, b->y, b->x + b->w - 1, b->y + b->h - 1, 55, 55, 55);\n\n    scale = (b->font_size + 7U) / 8U;\n    if (scale < 1) scale = 1;\n    if (scale > 8) scale = 8;\n    TableClipText(b->text, clipped, sizeof(clipped), b->w - 8, scale);\n    text_w = NStrLen(clipped) * 8U * scale;\n    text_h = 8U * scale;\n    tx = b->x + (b->w - (int32_t)text_w) / 2;\n    ty = b->y + (b->h - (int32_t)text_h) / 2;\n    if (tx < b->x + 2) tx = b->x + 2;\n    if (ty < b->y + 2) ty = b->y + 2;\n    HostText((uint32_t)tx, (uint32_t)ty, clipped,\n             b->text_r, b->text_g, b->text_b, scale);\n}\n\nstatic int ButtonPollClicked(NyotaButton *b) {\n    int32_t mx = 0, my = 0;\n    uint8_t buttons, down;\n    int inside, clicked;\n    if (!b) return 0;\n    if (!g_host || !g_host->pointer_state) {\n        OutError(\"BUTTON_CLICKED: host nie obsluguje wskaznika\");\n        return -1;\n    }\n    buttons = HostPointerState(&mx, &my);\n    down = buttons & 1U;\n    inside = mx >= b->x && my >= b->y && mx < b->x + b->w && my < b->y + b->h;\n    clicked = down && !b->prev_down && inside;\n    b->prev_down = down;\n    return clicked ? 1 : 0;\n}\n\n// ============================================================\n// POMIŃ BLOK (skocz za blok wcięty o więcej niż cur_indent)\n",
)

# BUTTON statement before VAR, next to TABLE.
replace_once(
    "src/nyota.c",
    "    // --- VAR ---\n    if (NStartsWith(line, \"VAR\")) {\n",
    "    // --- BUTTON nazwa, x, y, w, h, \"tekst\", \"font\", rozmiar, tr, tg, tb, br, bg, bb ---\n    if (PeekWord(line, \"BUTTON\")) {\n        const char *p = NTrim(line + 6);\n        char bname[64];\n        uint32_t bn = ParseIdent(p, bname, sizeof(bname));\n        char args[14][MAX_STR_LEN];\n        int n;\n        int32_t x, y, w, h, fsize, tr, tg, tb, br, bg, bb;\n        NyotaVal textv, fontv;\n        NyotaButton temp, *b;\n        if (!g_is_graphics) { OutError(\"BUTTON wymaga GRAPH\"); return; }\n        if (!bname[0]) { OutError(\"BUTTON: brak nazwy kontrolki\"); return; }\n        p = NTrim(p + bn);\n        if (*p != ',') { OutError(\"BUTTON: po nazwie wymagany przecinek\"); return; }\n        p = NTrim(p + 1);\n        n = SplitFunctionArgs(p, args, 14);\n        if (n != 13) {\n            OutError(\"BUTTON: wymagane x,y,w,h,tekst,font,rozmiar,RGB tekstu,RGB tla\");\n            return;\n        }\n        if (!ButtonEvalInt(args[0], &x) || !ButtonEvalInt(args[1], &y) ||\n            !ButtonEvalInt(args[2], &w) || !ButtonEvalInt(args[3], &h) ||\n            !ButtonEvalInt(args[6], &fsize) || !ButtonEvalInt(args[7], &tr) ||\n            !ButtonEvalInt(args[8], &tg) || !ButtonEvalInt(args[9], &tb) ||\n            !ButtonEvalInt(args[10], &br) || !ButtonEvalInt(args[11], &bg) ||\n            !ButtonEvalInt(args[12], &bb)) return;\n        if (x < 0 || y < 0 || w < 8 || h < 8) {\n            OutError(\"BUTTON: nieprawidlowa pozycja lub rozmiar\");\n            return;\n        }\n        if (fsize <= 0 || fsize > 64) {\n            OutError(\"BUTTON: rozmiar czcionki poza zakresem 1..64\");\n            return;\n        }\n        if (tr < 0 || tr > 255 || tg < 0 || tg > 255 || tb < 0 || tb > 255 ||\n            br < 0 || br > 255 || bg < 0 || bg > 255 || bb < 0 || bb > 255) {\n            OutError(\"BUTTON: kolory RGB wymagaja zakresu 0..255\");\n            return;\n        }\n        textv = Eval(args[4]);\n        fontv = Eval(args[5]);\n        if (textv.type != TYPE_STR) { OutError(\"BUTTON: tekst wymaga STRING\"); return; }\n        if (fontv.type != TYPE_STR || !fontv.s[0]) {\n            OutError(\"BUTTON: nazwa czcionki wymaga niepustego STRING\");\n            return;\n        }\n        temp.x = x; temp.y = y; temp.w = w; temp.h = h;\n        temp.font_size = (uint32_t)fsize;\n        temp.text_r = (uint8_t)tr; temp.text_g = (uint8_t)tg; temp.text_b = (uint8_t)tb;\n        temp.bg_r = (uint8_t)br; temp.bg_g = (uint8_t)bg; temp.bg_b = (uint8_t)bb;\n        temp.prev_down = 0;\n        NStrCopy(temp.name, bname, sizeof(temp.name));\n        NStrCopy(temp.text, textv.s, sizeof(temp.text));\n        NStrCopy(temp.font, fontv.s, sizeof(temp.font));\n        b = GetOrCreateButton(bname);\n        if (!b) return;\n        *b = temp;\n        RenderButton(b);\n        return;\n    }\n\n    // --- VAR ---\n    if (NStartsWith(line, \"VAR\")) {\n",
)

# Reset named widgets on each run. Also fix TABLE reset on standalone AyoOS path.
replace_once(
    "src/nyota.c",
    "    g_list_pool_used = 0;\n    g_table_count = 0;\n    g_is_graphics = 0;\n",
    "    g_list_pool_used = 0;\n    g_table_count = 0;\n    g_button_count = 0;\n    g_is_graphics = 0;\n",
)
replace_once(
    "src/nyota.c",
    "    g_error_handler_proc = 0xFFFFFFFF;\n    g_list_pool_used = 0;\n    for (int i = 0; i < MAX_NN; i++) g_nn[i].active = 0;\n",
    "    g_error_handler_proc = 0xFFFFFFFF;\n    g_list_pool_used = 0;\n    g_table_count = 0;\n    g_button_count = 0;\n    for (int i = 0; i < MAX_NN; i++) g_nn[i].active = 0;\n",
)


# ---------------- docs/nyota.md ----------------
replace_once(
    "docs/nyota.md",
    "- <span style=\"color: #006A4E;\">TABLE: nazwana kontrolka prezentacji danych; kolumny i szerokości, czcionka/rozmiar, kolor tekstu, opcjonalne źródło LIST/TUPLE/MARK oraz TABLE_DATA wykonane 2026-09-17</span>\n",
    "- <span style=\"color: #006A4E;\">TABLE: nazwana kontrolka prezentacji danych; kolumny i szerokości, czcionka/rozmiar, kolor tekstu, opcjonalne źródło LIST/TUPLE/MARK oraz TABLE_DATA wykonane 2026-09-17</span>\n- <span style=\"color: #006A4E;\">BUTTON: nazwana kontrolka GUI z geometrią, tekstem, czcionką/rozmiarem, kolorami tekstu/tła i BUTTON_CLICKED() wykonane 2026-09-17</span>\n",
)
replace_once(
    "docs/nyota.md",
    "## 44. GOTOXY\n",
    "## 43a. BUTTON\n\n`BUTTON` jest nazwaną kontrolką GUI. Nie jest typem zmiennej Nyoty.\n\n```nyota\nBUTTON zapisz, 40, 40, 160, 48, \"Zapisz\", \"SYSTEM\", 14, 255, 255, 255, 40, 110, 180\n```\n\nSkładnia:\n\n```text\nBUTTON nazwa, x, y, szerokosc, wysokosc, tekst, font, rozmiar,\n       text_r, text_g, text_b, bg_r, bg_g, bg_b\n```\n\nNazwa jest logicznym identyfikatorem kontrolki, analogicznie do `TABLE`; nie jest\nzmienną i nie jest automatycznie wyświetlanym tytułem. Ponowne `BUTTON` z tą samą\nnazwą aktualizuje kontrolkę. `BUTTON` wymaga wcześniejszego `GRAPH`.\n\nKliknięcie sprawdza funkcja:\n\n```nyota\nIF BUTTON_CLICKED(zapisz):\n    # reakcja programu\n```\n\n`BUTTON_CLICKED()` zwraca `BOOLEAN` i wykrywa przejście lewego przycisku wskaźnika\nz puszczonego do wciśniętego wewnątrz kontrolki. Akceptowana jest też forma\n`BUTTON_CLICKED(\"zapisz\")`. Host musi dostarczać stan wskaźnika; backend POSIX/SDL2\njuż go udostępnia. Powiązanie wskaźnika AyoOS wymaga odpowiedniego callbacku hosta.\n\nParametr `font` jest częścią definicji kontrolki. Bieżący prymityw tekstowy hosta\nwybiera fizyczną czcionkę po stronie backendu; nazwana obsługa fontów będzie\nrozszerzeniem kontraktu hosta, bez zmiany składni `BUTTON`.\n\n---\n\n## 44. GOTOXY\n",
)


# ---------------- docs/nyota_v05.md ----------------
p = Path("docs/nyota_v05.md")
text = p.read_text(encoding="utf-8")
anchor = "## 6.15. Kierunki FUTURE\n"
if text.count(anchor) != 1:
    raise SystemExit("docs/nyota_v05.md: 6.15 anchor missing")
button_doc = '''## 6.14a. BUTTON — nazwana kontrolka GUI\n\n**Status wdrożenia:** FUTURE (pierwszy etap wykonany wcześniej na prośbę)  \n<span style="color: #006A4E;">nazwa logiczna, geometria, tekst, font/rozmiar, kolory tekstu/tła, renderowanie i BUTTON_CLICKED() na hoście POSIX wykonane 2026-09-17</span>\n\n`BUTTON` nie jest typem zmiennej. Nazwa identyfikuje kontrolkę, tak aby program\nmógł utrzymywać wiele przycisków jednocześnie.\n\n```nyota\nBUTTON zapisz, 40, 40, 160, 48, "Zapisz", "SYSTEM", 14, 255, 255, 255, 40, 110, 180\nIF BUTTON_CLICKED(zapisz):\n    PRINT "klik"\n```\n\nPierwszy etap definiuje kontrolkę i semantykę kliknięcia. Rozbudowany wspólny\nsystem zdarzeń GUI, focus, disabled/hover, tab-order i callbacki pozostają częścią\nprzyszłego standardu GUI.\n\n'''
p.write_text(text.replace(anchor, button_doc + anchor, 1), encoding="utf-8")


# ---------------- README.md ----------------
replace_once(
    "README.md",
    "| `TABLE` | ✅ | nazwana kontrolka prezentacji LIST/TUPLE/MARK w trybie graficznym |\n",
    "| `TABLE` | ✅ | nazwana kontrolka prezentacji LIST/TUPLE/MARK w trybie graficznym |\n| `BUTTON` | ✅ | nazwana kontrolka GUI; wygląd + wykrywanie kliknięcia na hoście POSIX |\n",
)


# ---------------- tests ----------------
tests = {
    "tests/graph_button_basic.nyo": '''# Oczekiwane: OK\nBEGIN\nGRAPH 1\nBUTTON zapisz, 20, 20, 160, 48, "Zapisz", "SYSTEM", 14, 255, 255, 255, 40, 110, 180\nEND\n''',
    "tests/graph_button_reuse.nyo": '''# Oczekiwane: OK\nBEGIN\nGRAPH 1\nBUTTON akcja, 20, 20, 120, 40, "A", "SYSTEM", 12, 255, 255, 255, 30, 80, 140\nBUTTON akcja, 30, 30, 180, 50, "B", "SYSTEM", 14, 250, 250, 250, 70, 90, 120\nEND\n''',
    "tests/graph_button_clicked.nyo": '''# Oczekiwane: OK\nBEGIN\nGRAPH 1\nBUTTON ok, 20, 20, 120, 40, "OK", "SYSTEM", 12, 255, 255, 255, 30, 120, 60\nVAR klik := BUTTON_CLICKED(ok)\nEND\n''',
    "tests/graph_button_clicked_string.nyo": '''# Oczekiwane: OK\nBEGIN\nGRAPH 1\nBUTTON ok, 20, 20, 120, 40, "OK", "SYSTEM", 12, 255, 255, 255, 30, 120, 60\nVAR klik := BUTTON_CLICKED("ok")\nEND\n''',
    "tests/graph_button_bad_color.nyo": '''# Oczekiwane: BLAD\nBEGIN\nGRAPH 1\nBUTTON bad, 20, 20, 120, 40, "Bad", "SYSTEM", 12, 300, 255, 255, 30, 120, 60\nEND\n''',
    "tests/graph_button_bad_size.nyo": '''# Oczekiwane: BLAD\nBEGIN\nGRAPH 1\nBUTTON bad, 20, 20, 4, 4, "Bad", "SYSTEM", 12, 255, 255, 255, 30, 120, 60\nEND\n''',
    "tests/graph_button_unknown_clicked.nyo": '''# Oczekiwane: BLAD\nBEGIN\nGRAPH 1\nVAR klik := BUTTON_CLICKED(brak)\nEND\n''',
    "tests/button_without_graph.nyo": '''# Oczekiwane: BLAD\nBEGIN\nBUTTON bad, 20, 20, 120, 40, "Bad", "SYSTEM", 12, 255, 255, 255, 30, 120, 60\nEND\n''',
}
for path, content in tests.items():
    Path(path).write_text(content, encoding="utf-8")

print("BUTTON widget upgrade applied")
