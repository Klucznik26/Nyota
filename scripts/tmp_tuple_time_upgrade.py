from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one match, got {count}\n--- OLD ---\n{old[:500]}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


def insert_before(path, anchor, text_to_insert):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    if text.count(anchor) != 1:
        raise SystemExit(f"{path}: anchor count for {anchor!r} != 1")
    p.write_text(text.replace(anchor, text_to_insert + anchor, 1), encoding="utf-8")


src = "src/nyota.c"

replace_once(src,
'''#define TYPE_MARK    7
''',
'''#define TYPE_MARK    7
#define TYPE_TUPLE   8
#define TYPE_TIME    9
''')

replace_once(src,
'''static uint64_t HostUnixTime(void) {
    if (g_host && g_host->unix_time) return g_host->unix_time();
    return 0;
}
''',
'''static uint64_t HostUnixTime(void) {
    if (g_host && g_host->unix_time) return g_host->unix_time();
    return 0;
}

static uint32_t HostLocalTimeSeconds(void) {
    if (g_host && g_host->local_time_seconds) return g_host->local_time_seconds();
    return (uint32_t)(HostUnixTime() % 86400ULL);
}
''')

replace_once(src,
'''    if (t == TYPE_DATE) return "DATE";
    if (t == TYPE_MARK) return "MARK";
''',
'''    if (t == TYPE_DATE) return "DATE";
    if (t == TYPE_MARK) return "MARK";
    if (t == TYPE_TUPLE) return "TUPLE";
    if (t == TYPE_TIME) return "TIME";
''')

insert_before(src,
'''static int ParseDateLit(const char *p, int32_t *y, int32_t *m, int32_t *d, uint32_t *consumed) {
''',
'''static void ValFromTimeSeconds(NyotaVal *v, int64_t seconds) {
    seconds %= 86400;
    if (seconds < 0) seconds += 86400;
    ValClear(v);
    v->type = TYPE_TIME;
    v->i = (int32_t)seconds;
}

''')

replace_once(src,
'''    if (a->type == TYPE_INT || a->type == TYPE_BOOL || a->type == TYPE_DATE)
        return a->i == b->i;
''',
'''    if (a->type == TYPE_INT || a->type == TYPE_BOOL || a->type == TYPE_DATE || a->type == TYPE_TIME)
        return a->i == b->i;
''')

replace_once(src,
'''    if (a->type == TYPE_LIST) {
        if (a->list_len != b->list_len) return 0;
        for (uint32_t i = 0; i < a->list_len; i++) {
            if (!ValEqual(&a->list_items[i], &b->list_items[i])) return 0;
        }
        return 1;
    }
''',
'''    if (a->type == TYPE_LIST || a->type == TYPE_TUPLE) {
        if (a->list_len != b->list_len) return 0;
        for (uint32_t i = 0; i < a->list_len; i++) {
            if (!ValEqual(&a->list_items[i], &b->list_items[i])) return 0;
        }
        return 1;
    }
''')

replace_once(src,
'''    } else if (v->type == TYPE_DATE) {
''',
'''    } else if (v->type == TYPE_TIME) {
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
''')

replace_once(src,
'''    } else if (v->type == TYPE_LIST) {
        NStrCopy(out, "[", max);
        for (uint32_t i = 0; i < v->list_len; i++) {
            if (i > 0) NStrAppend(out, ", ", max);
            char tmp[128];
            ValToStr(&v->list_items[i], tmp, sizeof(tmp));
            NStrAppend(out, tmp, max);
        }
        NStrAppend(out, "]", max);
''',
'''    } else if (v->type == TYPE_LIST || v->type == TYPE_TUPLE) {
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
''')

replace_once(src,
'''    if (base.type == TYPE_LIST) {
        int32_t i;
        if (idx.type != TYPE_INT) {
            OutError("Indeks LIST wymaga INTEGER");
            return none;
        }
        i = idx.i;
        if (i >= 0 && (uint32_t)i < base.list_len)
            return base.list_items[i];
        OutError("Indeks listy poza zakresem");
        return none;
    }
''',
'''    if (base.type == TYPE_LIST || base.type == TYPE_TUPLE) {
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
''')

replace_once(src,
'''    if (!lst || lst->type != TYPE_LIST) return 0;
    if (need <= lst->list_cap) return 1;
    if (need > MAX_LIST_ITEMS) {
        OutError("LIST: przekroczono limit implementacji 64 elementow");
''',
'''    if (!lst || (lst->type != TYPE_LIST && lst->type != TYPE_TUPLE)) return 0;
    if (need <= lst->list_cap) return 1;
    if (need > MAX_LIST_ITEMS) {
        OutError("Sekwencja: przekroczono limit implementacji 64 elementow");
''')

insert_before(src,
'''static NyotaVal ListConcat(const NyotaVal *a, const NyotaVal *b) {
''',
'''static NyotaVal SeqCloneAs(const NyotaVal *src, uint8_t dst_type) {
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

''')

replace_once(src,
'''    if (a->type == TYPE_INT || a->type == TYPE_DATE) {
''',
'''    if (a->type == TYPE_INT || a->type == TYPE_DATE || a->type == TYPE_TIME) {
''')

replace_once(src,
'''        OutError("DATE obsluguje tylko + i -");
        return r;
    }
    if (a->type != b->type) {
''',
'''        OutError("DATE obsluguje tylko + i -");
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
''')

replace_once(src,
'''        else if (!in_str && (*q == '(' || *q == '[')) depth++;
        else if (!in_str && (*q == ')' || *q == ']')) {
''',
'''        else if (!in_str && (*q == '(' || *q == '[' || *q == '{')) depth++;
        else if (!in_str && (*q == ')' || *q == ']' || *q == '}')) {
''')

insert_before(src,
'''// Prosta ewaluacja wyrażenia (bez rekurencji dla nawiasów — linearny parser)
static NyotaVal ParsePrimary(const char **pp) {
''',
'''static int ParenLooksLikeTuple(const char *expr) {
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
''')

replace_once(src,
'''    if (expr[0] == '(') {
        const char *inner = expr + 1;
        result = ParseOr((const char **)&inner);
        *pp = MatchParen(expr);
        return result;
    }
''',
'''    if (expr[0] == '(') {
        if (ParenLooksLikeTuple(expr)) return ParseTupleLiteral(pp);
        {
            const char *inner = expr + 1;
            result = ParseOr((const char **)&inner);
            *pp = MatchParen(expr);
            return result;
        }
    }
''')

insert_before(src,
'''    if (NStrEqN(expr, "YEAR(", 5) || NStrEqN(expr, "MONTH(", 6) || NStrEqN(expr, "DAY(", 4)) {
''',
'''    if (NStrEqN(expr, "TIME(", 5)) {
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
''')

insert_before(src,
'''    if (NStrEqN(expr, "LEN(", 4)) {
''',
'''    if (NStrEqN(expr, "TUPLE(", 6) || NStrEqN(expr, "LIST(", 5)) {
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
''')

replace_once(src,
'''        else if (inner.type == TYPE_LIST || inner.type == TYPE_MARK)
            len = (int32_t)inner.list_len;
        else {
            OutError("LEN() wymaga STRING, LIST albo MARK");
''',
'''        else if (inner.type == TYPE_LIST || inner.type == TYPE_TUPLE || inner.type == TYPE_MARK)
            len = (int32_t)inner.list_len;
        else {
            OutError("LEN() wymaga STRING, LIST, TUPLE albo MARK");
''')

replace_once(src,
'''    if (v->type == TYPE_LIST) return v->list_len > 0;
    if (v->type == TYPE_DATE) return 1;
''',
'''    if (v->type == TYPE_LIST || v->type == TYPE_TUPLE) return v->list_len > 0;
    if (v->type == TYPE_DATE || v->type == TYPE_TIME) return 1;
''')

replace_once(src,
'''        (lv->type != TYPE_INT && lv->type != TYPE_FLOAT && lv->type != TYPE_DATE)) {
        OutError("Porownanie < > wymaga INTEGER, FLOAT albo DATE tego samego typu");
''',
'''        (lv->type != TYPE_INT && lv->type != TYPE_FLOAT && lv->type != TYPE_DATE && lv->type != TYPE_TIME)) {
        OutError("Porownanie < > wymaga INTEGER, FLOAT, DATE albo TIME tego samego typu");
''')

replace_once(src,
'''            if (right.type == TYPE_LIST) {
''',
'''            if (right.type == TYPE_LIST || right.type == TYPE_TUPLE) {
''')

replace_once(src,
'''            OutError("IN wymaga MARK albo LIST po prawej stronie");
''',
'''            OutError("IN wymaga MARK, LIST albo TUPLE po prawej stronie");
''')

replace_once(src,
'''static NyotaVal ParseAdd(const char **pp) {
    NyotaVal left = ParseMul(pp);
    for (;;) {
        const char *p = NTrim(*pp);
        if (p[0] != '+' && p[0] != '-') break;
        char op = p[0];
        *pp = p + 1;
        NyotaVal right = ParseMul(pp);
        left = ValArith(&left, op, &right);
    }
    return left;
}
''',
'''static NyotaVal ParseAdd(const char **pp) {
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
''')

replace_once(src,
'''            if (v && (v->val.type == TYPE_LIST || v->val.type == TYPE_MARK)) {
                if (v->is_const) { OutError("Nie mozna zmienic stalej kolekcji"); return; }
                v->val.list_len = 0;
                return;
            }
''',
'''            if (v && v->val.type == TYPE_TUPLE) {
                OutError("TUPLE jest niemutowalne");
                return;
            }
            if (v && (v->val.type == TYPE_LIST || v->val.type == TYPE_MARK)) {
                if (v->is_const) { OutError("Nie mozna zmienic stalej kolekcji"); return; }
                v->val.list_len = 0;
                return;
            }
''')

replace_once(src,
'''                if (v->val.type == TYPE_LIST && !idx2[0]) {
''',
'''                if (v->val.type == TYPE_TUPLE) {
                    OutError("TUPLE jest niemutowalne");
                } else if (v->val.type == TYPE_LIST && !idx2[0]) {
''')

replace_once(src,
'''            if (seq.type != TYPE_LIST) { OutError("FOR ... IN wymaga LIST"); return; }
''',
'''            if (seq.type != TYPE_LIST && seq.type != TYPE_TUPLE) { OutError("FOR ... IN wymaga LIST albo TUPLE"); return; }
''')

replace_once(src,
'''            if (t != TYPE_INT && t != TYPE_FLOAT && t != TYPE_STR && t != TYPE_DATE) {
                OutError("SORT: obslugiwane typy to INTEGER, FLOAT, STRING, DATE");
''',
'''            if (t != TYPE_INT && t != TYPE_FLOAT && t != TYPE_STR && t != TYPE_DATE && t != TYPE_TIME) {
                OutError("SORT: obslugiwane typy to INTEGER, FLOAT, STRING, DATE, TIME");
''')

# Host contract: local time-of-day, so TIME() is local on POSIX and can be native on AyoOS.
replace_once("src/nyota_host.h",
'''    uint64_t (*unix_time)(void);
    uint64_t (*ticks_100hz)(void);
''',
'''    uint64_t (*unix_time)(void);
    uint32_t (*local_time_seconds)(void);
    uint64_t (*ticks_100hz)(void);
''')

replace_once("host/posix/host.c",
'''static uint64_t host_unix(void) {
    return (uint64_t)time(NULL);
}
''',
'''static uint64_t host_unix(void) {
    return (uint64_t)time(NULL);
}

static uint32_t host_local_time_seconds(void) {
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    return (uint32_t)(tmv.tm_hour * 3600 + tmv.tm_min * 60 + tmv.tm_sec);
}
''')

replace_once("host/posix/host.c",
'''    g_nyhost.unix_time = host_unix;
    g_nyhost.ticks_100hz = host_ticks;
''',
'''    g_nyhost.unix_time = host_unix;
    g_nyhost.local_time_seconds = host_local_time_seconds;
    g_nyhost.ticks_100hz = host_ticks;
''')

# Documentation: add TUPLE as a first-class immutable sequence and close TIME semantics.
doc = Path("docs/nyota_v05.md")
text = doc.read_text(encoding="utf-8")
for old, new in [
    ("## 6.14. Kierunki FUTURE", "## 6.15. Kierunki FUTURE"),
    ("## 6.13. TIME — pełnoprawny typ czasu", "## 6.14. TIME — pełnoprawny typ czasu"),
    ("## 6.12. DATE — pełnoprawny typ daty", "## 6.13. DATE — pełnoprawny typ daty"),
    ("## 6.11. Rozszerzenia MARK", "## 6.12. Rozszerzenia MARK"),
    ("## 6.10. Losowe listy liczbowe", "## 6.11. Losowe listy liczbowe"),
    ("## 6.9. Rozszerzenia LIST", "## 6.10. Rozszerzenia LIST"),
]:
    if text.count(old) != 1:
        raise SystemExit(f"docs/nyota_v05.md: heading {old!r} count != 1")
    text = text.replace(old, new, 1)
anchor = "## 6.10. Rozszerzenia LIST"
tuple_doc = '''## 6.9. TUPLE — niemutowalna sekwencja\n\n**Status wdrożenia:** APPROVED AFTER CORE  \n<span style="color: #006A4E;">literał, indeksowanie, LEN, IN, FOR...IN oraz konwersje LIST/TUPLE wykonane 2026-09-17</span>\n\n`TUPLE` jest uporządkowaną, niemutowalną sekwencją. Może zawierać wartości różnych typów.\n\n```nyota\nVAR a := ()\nVAR b := (5,)\nVAR c := (10, "Ayo", TRUE)\n```\n\nPrzecinek odróżnia jednoelementową `TUPLE` od zwykłego nawiasu grupującego.\n\n```nyota\n(5)     # INTEGER w nawiasie\n(5,)    # TUPLE z jednym elementem\n```\n\nDostęp i iteracja:\n\n```nyota\nPRINT c[0]\nPRINT LEN(c)\nFOR x IN c:\n    PRINT x\n```\n\nOperator `IN` sprawdza obecność wartości z zachowaniem ścisłej tożsamości typu.\n\n`TUPLE` jest niemutowalne. Niedozwolone są przypisanie do indeksu oraz instrukcje mutujące `APPEND`, `REMOVE`, `EXTEND`, `CLEAR`, `REVERSE` i `SORT`.\n\nKonwersje tworzą nowy kontener:\n\n```nyota\nVAR lista := [1, 2, 3]\nVAR t := TUPLE(lista)\nVAR kopia := LIST(t)\n```\n\nZmiana `lista` lub `kopia` nie zmienia długości ani układu `t`.\n\n'''
if text.count(anchor) != 1:
    raise SystemExit("docs/nyota_v05.md: LIST anchor missing")
text = text.replace(anchor, tuple_doc + anchor, 1)
text = text.replace(
'''**Status wdrożenia:** APPROVED AFTER CORE  \n**Brama implementacji:** nie wdrażać, dopóki nie są zamknięte `TIME - TIME`\noraz decyzja o `DATETIME`.\n''',
'''**Status wdrożenia:** APPROVED AFTER CORE  \n<span style="color: #006A4E;">TIME(), TIME(HH.MM.SS), HOUR/MINUTE/SECOND, przesunięcia H/M/S, TIME-TIME, porównania i SORT wykonane 2026-09-17</span>\n''', 1)
text = text.replace(
'''Różnica `TIME - TIME` nie została jeszcze zdefiniowana. To nie jest drobiazg\ndo dopisania przy implementacji: bez tej reguły, bez decyzji o jednostce\nwyniku i bez decyzji o `DATETIME` interpreter nie dostaje typu `TIME`.\n''',
'''Różnica `TIME - TIME` zwraca `INTEGER` wyrażony w sekundach. Jest to różnica\nprostych wartości pory dnia bez automatycznego wybierania krótszej drogi przez północ.\n\n```nyota\nTIME(14.30.00) - TIME(13.00.00)   # 5400\nTIME(01.00.00) - TIME(23.00.00)   # -79200\n```\n\n`DATETIME` pozostaje osobnym kierunkiem FUTURE i nie zmienia tej reguły.\n''', 1)
doc.write_text(text, encoding="utf-8")

# Agent-facing status.
ny = Path("docs/nyota.md")
text = ny.read_text(encoding="utf-8")
needle = '- <span style="color: navy;">LIST: `+` `-` `><`, EXTEND, REMOVE po indeksie i wartości, CLEAR, REVERSE, SORT/DESC w toku (zaawansowany etap) 2026-09-17</span>\n'
if text.count(needle) != 1:
    raise SystemExit("docs/nyota.md: LIST status anchor missing")
text = text.replace(needle, needle + '- <span style="color: #006A4E;">TUPLE: literał, indeksowanie, LEN, IN, FOR...IN i konwersje LIST/TUPLE wykonane 2026-09-17</span>\n- <span style="color: #006A4E;">TIME: TIME(), TIME(HH.MM.SS), HOUR/MINUTE/SECOND, H/M/S, TIME-TIME, porównania i SORT wykonane 2026-09-17</span>\n', 1)
ny.write_text(text, encoding="utf-8")

# Regression tests.
tests = {
"tests/tuple_basic.nyo": '''# Oczekiwane: zawiera 3 Ayo\nBEGIN\nVAR t := (10, "Ayo", TRUE)\nPRINT LEN(t), t[1]\nEND\n''',
"tests/tuple_single.nyo": '''# Oczekiwane: zawiera (5,)\nBEGIN\nVAR t := (5,)\nPRINT t\nEND\n''',
"tests/tuple_nested.nyo": '''# Oczekiwane: zawiera 3 5\nBEGIN\nVAR t := (1, (2, 3), [4, 5])\nPRINT t[1][1], t[2][1]\nEND\n''',
"tests/tuple_convert.nyo": '''# Oczekiwane: zawiera 2 3 3\nBEGIN\nVAR a := [1, 2]\nVAR t := TUPLE(a)\nAPPEND a, 3\nVAR b := LIST(t)\nAPPEND b, 4\nPRINT LEN(t), LEN(a), LEN(b)\nEND\n''',
"tests/tuple_for.nyo": '''# Oczekiwane: zawiera 6\nBEGIN\nVAR suma := 0\nVAR t := (1, 2, 3)\nFOR x IN t:\n    suma := suma + x\nPRINT suma\nEND\n''',
"tests/tuple_in.nyo": '''# Oczekiwane: zawiera TRUE FALSE\nBEGIN\nVAR t := (1, "1", TRUE)\nPRINT 1 IN t, 2 IN t\nEND\n''',
"tests/tuple_immutable.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR t := (1, 2, 3)\nt[0] := 9\nEND\n''',
"tests/tuple_clear_forbidden.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR t := (1, 2, 3)\nCLEAR t\nEND\n''',
"tests/time_basic.nyo": '''# Oczekiwane: zawiera 14 20 05\nBEGIN\nVAR t := TIME(14.20.05)\nPRINT HOUR(t), MINUTE(t), SECOND(t)\nEND\n''',
"tests/time_invalid.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR t := TIME(24.00.00)\nEND\n''',
"tests/time_arith.nyo": '''# Oczekiwane: zawiera TIME(00.10.00) TIME(13.50.20)\nBEGIN\nVAR a := TIME(23.50.00) + M20\nVAR b := TIME(14.20.20) - M30\nPRINT a, b\nEND\n''',
"tests/time_units.nyo": '''# Oczekiwane: zawiera TIME(16.21.00)\nBEGIN\nVAR t := TIME(14.20.20) + H2 + S40\nPRINT t\nEND\n''',
"tests/time_diff.nyo": '''# Oczekiwane: zawiera 5400 -79200\nBEGIN\nPRINT TIME(14.30.00) - TIME(13.00.00), TIME(01.00.00) - TIME(23.00.00)\nEND\n''',
"tests/time_cmp.nyo": '''# Oczekiwane: zawiera TRUE TRUE TRUE\nBEGIN\nPRINT TIME(14.30.00) < TIME(16.00.00), TIME(20.00.00) > TIME(08.00.00), TIME(12.00.00) = TIME(12.00.00)\nEND\n''',
"tests/time_sort.nyo": '''# Oczekiwane: zawiera TIME(08.00.00)\nBEGIN\nVAR t := [TIME(18.00.00), TIME(08.00.00), TIME(12.30.00)]\nSORT t\nPRINT t[0]\nEND\n''',
"tests/time_now.nyo": '''# Oczekiwane: zawiera TRUE TRUE\nBEGIN\nVAR t := TIME()\nPRINT HOUR(t) >= 0, HOUR(t) <= 23\nEND\n''',
}
for path, content in tests.items():
    Path(path).write_text(content, encoding="utf-8")

# Permanent CI for future pushes and pull requests.
ci = Path(".github/workflows/ci.yml")
ci.parent.mkdir(parents=True, exist_ok=True)
ci.write_text('''name: Nyota CI\n\non:\n  push:\n    branches: [main]\n  pull_request:\n    branches: [main]\n\njobs:\n  build-test:\n    runs-on: ubuntu-latest\n    steps:\n      - uses: actions/checkout@v4\n      - name: Install dependencies\n        run: |\n          sudo apt-get update\n          sudo apt-get install -y build-essential pkg-config libsdl2-dev\n      - name: Build\n        run: make\n      - name: Regression tests\n        run: make test\n      - name: Headless graphics tests\n        run: make test-graph\n''', encoding="utf-8")

print("TUPLE/TIME upgrade applied")
