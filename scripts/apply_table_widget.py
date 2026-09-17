from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"{path}: expected exactly one match, got {n}\nANCHOR:\n{old[:240]}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# ---------------- src/nyota.c ----------------
p = Path("src/nyota.c")
text = p.read_text(encoding="utf-8")
if "// TABLE widget support" in text:
    print("TABLE widget already applied")
    raise SystemExit(0)

replace_once(
    "src/nyota.c",
    "#define MAX_LIST_ITEMS  64      // max elementów listy\n#define NYOTA_INDENT    4       // jeden poziom bloku = dokładnie 4 spacje\n",
    "#define MAX_LIST_ITEMS  64      // max elementów listy\n#define MAX_TABLES      32      // max nazwanych kontrolek TABLE\n#define MAX_TABLE_COLS  16      // max kolumn jednej TABLE\n#define NYOTA_INDENT    4       // jeden poziom bloku = dokładnie 4 spacje\n",
)

replace_once(
    "src/nyota.c",
    "typedef struct {\n    char     name[64];\n    uint32_t start_line;   // linia z PROCEDURE/FUNCTION\n    uint32_t body_line;    // pierwsza linia ciała\n    uint32_t param_count;\n    char     params[8][64];\n    uint8_t  param_by_ref[8];  // czy parametr przez referencję (VAR)\n    uint8_t  is_function;      // 1 = FUNCTION, 0 = PROCEDURE\n} NyotaProc;\n",
    "typedef struct {\n    char     name[64];\n    uint32_t start_line;   // linia z PROCEDURE/FUNCTION\n    uint32_t body_line;    // pierwsza linia ciała\n    uint32_t param_count;\n    char     params[8][64];\n    uint8_t  param_by_ref[8];  // czy parametr przez referencję (VAR)\n    uint8_t  is_function;      // 1 = FUNCTION, 0 = PROCEDURE\n} NyotaProc;\n\n// TABLE widget support: TABLE jest kontrolką prezentacji, nie typem Nyota.\ntypedef struct {\n    char name[64];\n    int32_t x, y, w, h;\n    uint32_t columns;\n    int32_t widths[MAX_TABLE_COLS];\n    char font[64];\n    uint32_t font_size;\n    uint8_t r, g, b;\n    char source_name[64];\n} NyotaTable;\n",
)

replace_once(
    "src/nyota.c",
    "static NyotaProc g_procs[MAX_PROCS];\nstatic uint32_t  g_proc_count = 0;\n",
    "static NyotaProc g_procs[MAX_PROCS];\nstatic uint32_t  g_proc_count = 0;\n\nstatic NyotaTable g_tables[MAX_TABLES];\nstatic uint32_t   g_table_count = 0;\n",
)

helpers = r'''
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

'''
replace_once(
    "src/nyota.c",
    "// ============================================================\n// POMIŃ BLOK (skocz za blok wcięty o więcej niż cur_indent)\n// ============================================================\n",
    helpers + "// ============================================================\n// POMIŃ BLOK (skocz za blok wcięty o więcej niż cur_indent)\n// ============================================================\n",
)

statement = r'''
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

'''
replace_once(
    "src/nyota.c",
    "    // --- VAR ---\n    if (NStartsWith(line, \"VAR\")) {\n",
    statement + "    // --- VAR ---\n    if (NStartsWith(line, \"VAR\")) {\n",
)

replace_once(
    "src/nyota.c",
    "    g_error_handler_proc = 0xFFFFFFFF;\n    g_list_pool_used = 0;\n    g_is_graphics = 0;\n",
    "    g_error_handler_proc = 0xFFFFFFFF;\n    g_list_pool_used = 0;\n    g_table_count = 0;\n    g_is_graphics = 0;\n",
)

# ---------------- scripts/run_tests.sh ----------------
replace_once(
    "scripts/run_tests.sh",
    "    graph_*)\n        extra_env=\"SDL_VIDEODRIVER=dummy NYOTA_NO_WAIT=1\"\n        ;;\n",
    "    graph_*|table_*)\n        extra_env=\"SDL_VIDEODRIVER=dummy NYOTA_NO_WAIT=1\"\n        ;;\n",
)

# ---------------- tests ----------------
tests = {
    "tests/table_empty.nyo": '''# Oczekiwane: OK\nBEGIN\nGRAPH 1\nTABLE pusta, 10, 10, 300, 120, 3, [100, 100, 100], "SYSTEM", 12, 220, 220, 220\nEND\n''',
    "tests/table_list.nyo": '''# Oczekiwane: OK\nBEGIN\nVAR dane := [10, 20, 30]\nGRAPH 1\nTABLE liczby, 10, 10, 200, 120, 1, [200], "SYSTEM", 12, 255, 255, 255, dane\nEND\n''',
    "tests/table_rows.nyo": '''# Oczekiwane: OK\nBEGIN\nVAR dane := [[1, "A"], [2, "B"], [3, "C"]]\nGRAPH 1\nTABLE osoby, 10, 10, 300, 140, 2, [80, 220], "SYSTEM", 14, 200, 240, 255, dane\nEND\n''',
    "tests/table_tuple_rows.nyo": '''# Oczekiwane: OK\nBEGIN\nVAR dane := [(1, "A"), (2, "B")]\nGRAPH 1\nTABLE tuple_rows, 10, 10, 240, 100, 2, [80, 160], "SYSTEM", 10, 255, 220, 180, dane\nEND\n''',
    "tests/table_mark.nyo": '''# Oczekiwane: OK\nBEGIN\nVAR kraje := {3| "Polska" | "Warszawa" | "Wisla", "Niemcy" | "Berlin" | "Ren" }\nGRAPH 1\nTABLE kraje_view, 10, 10, 450, 120, 3, [150, 180, 120], "SYSTEM", 12, 220, 255, 220, kraje\nEND\n''',
    "tests/table_data.nyo": '''# Oczekiwane: OK\nBEGIN\nVAR a := [1, 2, 3]\nVAR b := [4, 5, 6]\nGRAPH 1\nTABLE wynik, 10, 10, 200, 120, 1, [200], "SYSTEM", 12, 255, 255, 255, a\nTABLE_DATA wynik, b\nEND\n''',
    "tests/table_name_reuse.nyo": '''# Oczekiwane: OK\nBEGIN\nVAR a := [1, 2]\nGRAPH 1\nTABLE panel, 10, 10, 200, 100, 1, [200], "SYSTEM", 12, 255, 255, 255, a\nTABLE panel, 20, 20, 240, 100, 1, [240], "SYSTEM", 16, 200, 255, 200, a\nEND\n''',
    "tests/table_width_error.nyo": '''# Oczekiwane: BLAD\nBEGIN\nGRAPH 1\nTABLE zla, 10, 10, 300, 100, 3, [100, 100], "SYSTEM", 12, 255, 255, 255\nEND\n''',
    "tests/table_width_sum_error.nyo": '''# Oczekiwane: BLAD\nBEGIN\nGRAPH 1\nTABLE zla, 10, 10, 300, 100, 2, [100, 150], "SYSTEM", 12, 255, 255, 255\nEND\n''',
    "tests/table_shape_error.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR dane := [[1, 2], [3, 4, 5]]\nGRAPH 1\nTABLE zla, 10, 10, 300, 100, 2, [150, 150], "SYSTEM", 12, 255, 255, 255, dane\nEND\n''',
    "tests/table_without_graph.nyo": '''# Oczekiwane: BLAD\nBEGIN\nTABLE zla, 10, 10, 200, 100, 1, [200], "SYSTEM", 12, 255, 255, 255\nEND\n''',
}
for path, content in tests.items():
    Path(path).write_text(content, encoding="utf-8")

# ---------------- docs/nyota.md ----------------
replace_once(
    "docs/nyota.md",
    '- <span style="color: #006A4E;">TIME: TIME(), TIME(HH.MM.SS), HOUR/MINUTE/SECOND, H/M/S, TIME-TIME, porównania i SORT wykonane 2026-09-17</span>\n',
    '- <span style="color: #006A4E;">TIME: TIME(), TIME(HH.MM.SS), HOUR/MINUTE/SECOND, H/M/S, TIME-TIME, porównania i SORT wykonane 2026-09-17</span>\n- <span style="color: #006A4E;">TABLE: nazwana kontrolka prezentacji danych; kolumny i szerokości, czcionka/rozmiar, kolor tekstu, opcjonalne źródło LIST/TUPLE/MARK oraz TABLE_DATA wykonane 2026-09-17</span>\n',
)

# ---------------- docs/nyota_v05.md ----------------
doc = Path("docs/nyota_v05.md")
text = doc.read_text(encoding="utf-8")
text = text.replace("relacyjny MARK\nTABLE\nDATETIME", "relacyjny MARK\nDATETIME", 1)
old = "## 6.15. Kierunki FUTURE\n"
if text.count(old) != 1:
    raise SystemExit("docs/nyota_v05.md: FUTURE heading anchor missing")
section = r'''## 6.15. TABLE — nazwana kontrolka prezentacji danych

**Status wdrożenia:** FUTURE (pierwszy etap wykonany wcześniej na prośbę)  
<span style="color: #006A4E;">nazwana kontrolka, układ kolumn, font/rozmiar, kolor tekstu, LIST/TUPLE/MARK i TABLE_DATA wykonane 2026-09-17</span>

`TABLE` **nie jest typem zmiennej**. Jest kontrolką prezentacyjną działającą na
warstwie graficznej Nyoty. Dane pozostają w istniejących typach takich jak
`LIST`, `TUPLE` i `MARK`.

Każda tabela ma obowiązkową **nazwę logiczną**, dzięki której program może
odwoływać się do konkretnej kontrolki, gdy tabel jest wiele. Nazwa należy do
osobnej przestrzeni kontrolek TABLE i nie jest zmienną Nyoty.

Podstawowa składnia:

```nyota
TABLE kraje_view, 20, 60, 700, 300, 3, [180, 260, 260], "SYSTEM", 14, 220, 220, 220
```

Parametry oznaczają kolejno:

```text
nazwa tabeli
x, y
szerokosc, wysokosc
liczba kolumn
LIST/TUPLE szerokosci poszczegolnych kolumn
nazwa czcionki
rozmiar czcionki
R, G, B koloru tekstu
opcjonalne zrodlo danych
```

W pierwszym etapie suma szerokości kolumn musi być dokładnie równa szerokości
kontrolki. Każda szerokość jest dodatnim `INTEGER`, a liczba pozycji w liście
szerokości musi odpowiadać liczbie kolumn.

Źródło można podać od razu:

```nyota
TABLE kraje_view, 20, 60, 700, 300, 3, [180, 260, 260], "SYSTEM", 14, 220, 220, 220, kraje
```

albo później zmienić je przez nazwę tabeli:

```nyota
TABLE_DATA kraje_view, inne_dane
```

Źródło jest nazwą istniejącej zmiennej `LIST`, `TUPLE` albo `MARK`; `TABLE` nie
kopiuje danych do nowego typu.

Zasady prezentacji pierwszej wersji:

```text
MARK             kolumna 0 = klucz, dalej kolumny wartosci
LIST/TUPLE       jedna kolumna -> elementy jako kolejne wiersze
LIST/TUPLE       wiele kolumn -> elementy musza byc wierszami LIST/TUPLE
brak zrodla       pusta kontrolka z ukladem kolumn
```

Dla `MARK` liczba kolumn TABLE musi odpowiadać: `klucz + kolumny wartości`.
Dla wielokolumnowych `LIST/TUPLE` każdy wiersz musi mieć dokładnie tyle pól,
ile zadeklarowano kolumn.

Ponowne wykonanie `TABLE` z tą samą nazwą aktualizuje tę samą logiczną kontrolkę,
a nie tworzy drugiej o nierozróżnialnym identyfikatorze.

Nazwa fontu jest częścią kontraktu kontrolki. Host może użyć fontu zastępczego,
jeżeli nie posiada wskazanej czcionki; referencyjny host POSIX nadal używa
wbudowanego fontu bitmapowego i skaluje go do żądanego rozmiaru.

Na tym etapie TABLE nie zapewnia jeszcze nagłówków, edycji komórek, zaznaczania,
przewijania ani sortowania kliknięciem. To są późniejsze możliwości GUI, nie
warunek istnienia podstawowej kontrolki prezentacyjnej.

'''
text = text.replace(old, section + "## 6.16. Kierunki FUTURE\n", 1)
text = text.replace("relacyjny MARK\nTABLE\nDATETIME", "relacyjny MARK\nDATETIME", 1)
doc.write_text(text, encoding="utf-8")

# ---------------- README ----------------
readme = Path("README.md")
text = readme.read_text(encoding="utf-8")
old_row = "| `MARK` | 🚧 | autorska struktura tabelowa Nyoty |\n"
if old_row not in text:
    raise SystemExit("README: MARK row anchor missing")
text = text.replace(old_row, old_row + "| `TABLE` | ✅ | nazwana kontrolka prezentacji LIST/TUPLE/MARK w trybie graficznym |\n", 1)
text = text.replace("relacyjnego `MARK`, rozbudowanego `TABLE`, standardowego GUI", "relacyjnego `MARK`, dalszego rozwoju `TABLE`, standardowego GUI", 1)
readme.write_text(text, encoding="utf-8")

print("TABLE widget upgrade applied")
