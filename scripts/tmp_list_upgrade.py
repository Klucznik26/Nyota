from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one match, got {count}\n--- OLD ---\n{old[:500]}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


src = "src/nyota.c"

# Argument splitting must understand nested MARK/LIST/function expressions.
replace_once(src,
'''        if (!in_str && (*p == '(' || *p == '[')) depth++;
        if (!in_str && (*p == ')' || *p == ']')) { 
''',
'''        if (!in_str && (*p == '(' || *p == '[' || *p == '{')) depth++;
        if (!in_str && (*p == ')' || *p == ']' || *p == '}')) { 
''')

# LIST indexes are intentionally strict: only INTEGER is accepted.
replace_once(src,
'''    if (base.type == TYPE_LIST) {
        int32_t i = ValToInt(&idx);
        if (i >= 0 && (uint32_t)i < base.list_len)
            return base.list_items[i];
        OutError("Indeks listy poza zakresem");
        return none;
    }
''',
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
''')

# Add safe growth for mutable lists. The static pool is monotonic for one run,
# so growing allocates a new block and copies the old contents.
replace_once(src,
'''static void ListRemoveAt(NyotaVal *lst, uint32_t i) {
    uint32_t j;
    if (!lst || i >= lst->list_len) return;
    for (j = i; j + 1 < lst->list_len; j++)
        lst->list_items[j] = lst->list_items[j + 1];
    lst->list_len--;
}

static NyotaVal ListClone(const NyotaVal *src) {
''',
'''static void ListRemoveAt(NyotaVal *lst, uint32_t i) {
    uint32_t j;
    if (!lst || i >= lst->list_len) return;
    for (j = i; j + 1 < lst->list_len; j++)
        lst->list_items[j] = lst->list_items[j + 1];
    lst->list_len--;
}

static int ListEnsureCapacity(NyotaVal *lst, uint32_t need) {
    NyotaVal *items;
    uint32_t cap, i;
    if (!lst || lst->type != TYPE_LIST) return 0;
    if (need <= lst->list_cap) return 1;
    if (need > MAX_LIST_ITEMS) {
        OutError("LIST: przekroczono limit implementacji 64 elementow");
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
''')

# Operators return a new LIST but still respect the current implementation limit.
replace_once(src,
'''    n = a->list_len + b->list_len;
    r.type = TYPE_LIST;
''',
'''    n = a->list_len + b->list_len;
    if (n > MAX_LIST_ITEMS) {
        OutError("LIST: wynik ma wiecej niz 64 elementy");
        return r;
    }
    r.type = TYPE_LIST;
''')

# BOOLEAN is comparable for equality but is not an ordered SORT type.
replace_once(src,
'''    if (a->type == TYPE_INT || a->type == TYPE_BOOL || a->type == TYPE_DATE) {
''',
'''    if (a->type == TYPE_INT || a->type == TYPE_DATE) {
''')

# Replace the literal parser: nested lists/calls/MARK values work and capacity
# grows on demand instead of reserving 64 slots for every literal.
replace_once(src,
'''    // Lista [...]
    if (expr[0] == '[') {
        result.type = TYPE_LIST;
        result.list_items = (g_list_pool_used + MAX_LIST_ITEMS <= LIST_POOL_SIZE)
                             ? &g_list_pool[g_list_pool_used] : 0;
        result.list_cap = result.list_items ? MAX_LIST_ITEMS : 0;
        result.list_len = 0;
        if (result.list_items) g_list_pool_used += MAX_LIST_ITEMS;
        const char *p = NTrim(expr + 1);
        while (*p && *p != ']' && result.list_len < MAX_LIST_ITEMS) {
            // Parsuj element
            char elem_buf[MAX_STR_LEN];
            uint32_t ei = 0;
            // Zbierz do przecinka lub ]
            int in_str = 0;
            while (*p && !(!in_str && (*p == ',' || *p == ']')) && ei + 1 < MAX_STR_LEN) {
                if (*p == '"') in_str = !in_str;
                elem_buf[ei++] = *p++;
            }
            elem_buf[ei] = '\0';
            NRTrim(elem_buf);
            if (result.list_items)
                result.list_items[result.list_len++] = Eval(elem_buf);
            if (*p == ',') p++;
            p = NTrim(p);
        }
        if (*p == ']') p++;
        *pp = p;
        return result;
    }
''',
'''    // Lista [...]
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
''')

# Add the approved list generators. RANDINT uses inclusive integer bounds.
# RANDFLT accepts numeric bounds, uses fixed-point millis and precision 0..3.
replace_once(src,
'''    if (NStrEqN(expr, "RANDOM(", 7)) {
''',
'''    if (NStrEqN(expr, "RANDINT(", 8)) {
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
''')

# CLEAR mutates collections, therefore it cannot mutate CONST.
replace_once(src,
'''            NyotaVar *v = FindVar(name);
            if (v && (v->val.type == TYPE_LIST || v->val.type == TYPE_MARK)) {
                v->val.list_len = 0;
                return;
            }
''',
'''            NyotaVar *v = FindVar(name);
            if (v && (v->val.type == TYPE_LIST || v->val.type == TYPE_MARK)) {
                if (v->is_const) { OutError("Nie mozna zmienic stalej kolekcji"); return; }
                v->val.list_len = 0;
                return;
            }
''')

# Indexed LIST assignment: no implicit index conversion and no CONST mutation.
replace_once(src,
'''                if (v->val.type == TYPE_LIST && !idx2[0]) {
                    NyotaVal idx = Eval(idx1);
                    int32_t i = ValToInt(&idx);
                    if (i >= 0 && (uint32_t)i < v->val.list_len)
                        v->val.list_items[i] = rhs;
                    else OutError("Indeks listy poza zakresem");
''',
'''                if (v->val.type == TYPE_LIST && !idx2[0]) {
                    NyotaVal idx = Eval(idx1);
                    int32_t i;
                    if (v->is_const) { OutError("Nie mozna zmienic stalej LIST"); return; }
                    if (idx.type != TYPE_INT) { OutError("Indeks LIST wymaga INTEGER"); return; }
                    i = idx.i;
                    if (i >= 0 && (uint32_t)i < v->val.list_len)
                        v->val.list_items[i] = rhs;
                    else OutError("Indeks listy poza zakresem");
''')

# FOR ... IN LIST is evaluated once and iterates in the current list order.
replace_once(src,
'''    // --- FOR i := START TO END [STEP n]: ---
    if (NStartsWith(line, "FOR")) {
        const char *p = NTrim(line + 3);
        char var_name[64]; uint32_t vlen = ParseIdent(p, var_name, sizeof(var_name));
        p = NTrim(p + vlen);
        if (p[0] != ':' || p[1] != '=') { OutError("FOR: brakuje :="); return; }
''',
'''    // --- FOR x IN lista:  albo  FOR i := START TO END [STEP n]: ---
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
            if (seq.type != TYPE_LIST) { OutError("FOR ... IN wymaga LIST"); return; }
            v = FindVar(var_name);
            if (v && v->is_const) { OutError("FOR ... IN nie moze uzyc CONST jako iteratora"); return; }
            if (!v) v = CreateVar(var_name);
            if (!v) { OutError("Za duzo zmiennych"); return; }
            my_indent = NIndent(raw);
            body_start = ln + 1;
            body_end = SkipBlock(body_start, my_indent);
            g_loop_depth++;
            for (li = 0; li < seq.list_len; li++) {
                v->val = seq.list_items[li];
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
''')

# APPEND: strict syntax, CONST protection and safe growth.
replace_once(src,
'''    // --- APPEND lista, element ---
    if (NStartsWith(line, "APPEND")) {
        const char *p = NTrim(line + 6);
        char list_name[64]; uint32_t nl = ParseIdent(p, list_name, sizeof(list_name));
        p = NTrim(p + nl);
        if (*p == ',') p = NTrim(p + 1);
        NyotaVar *v = FindVar(list_name);
        if (v && v->val.type == TYPE_LIST && v->val.list_len < MAX_LIST_ITEMS) {
            v->val.list_items[v->val.list_len++] = Eval(p);
        } else {
            OutError("APPEND: brak listy lub pelna");
        }
        return;
    }
''',
'''    // --- APPEND lista, element ---
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
''')

# REMOVE: strict integer indexes; robust top-level comma parsing; ALL is explicit.
replace_once(src,
'''    // --- REMOVE lista[index]  albo  REMOVE lista, wartosc [, ALL] ---
    if (NStartsWith(line, "REMOVE")) {
        const char *p = NTrim(line + 6);
        char list_name[64]; uint32_t nl = ParseIdent(p, list_name, sizeof(list_name));
        NyotaVar *v = FindVar(list_name);
        p = NTrim(p + nl);
        if (!v || v->val.type != TYPE_LIST) { OutError("REMOVE wymaga LIST"); return; }
        if (*p == '[') {
            char idx_buf[128];
            uint32_t ii = 0;
            p++;
            while (*p && *p != ']' && ii + 1 < sizeof(idx_buf)) idx_buf[ii++] = *p++;
            idx_buf[ii] = '\0';
            {
                NyotaVal idx = Eval(idx_buf);
                int32_t i = ValToInt(&idx);
                if (i < 0 || (uint32_t)i >= v->val.list_len) OutError("REMOVE: indeks poza zakresem");
                else ListRemoveAt(&v->val, (uint32_t)i);
            }
            return;
        }
        if (*p == ',') p = NTrim(p + 1);
        {
            char expr[256];
            uint32_t ei = 0;
            int all = 0;
            while (*p && ei + 1 < sizeof(expr)) {
                if (NStrEqN(p, ", ALL", 5) || NStrEqN(p, ",ALL", 4)) {
                    all = 1;
                    break;
                }
                expr[ei++] = *p++;
            }
            expr[ei] = '\0';
            NRTrim(expr);
            {
                NyotaVal val = Eval(expr);
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
        }
        return;
    }
''',
'''    // --- REMOVE lista[index]  albo  REMOVE lista, wartosc [, ALL] ---
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
''')

# EXTEND keeps strict typed uniqueness and grows safely.
replace_once(src,
'''    // --- EXTEND a, b ---
    if (NStartsWith(line, "EXTEND")) {
        const char *p = NTrim(line + 6);
        char na[64], nb[64];
        uint32_t n1 = ParseIdent(p, na, sizeof(na));
        p = NTrim(p + n1);
        if (*p == ',') p = NTrim(p + 1);
        ParseIdent(p, nb, sizeof(nb));
        {
            NyotaVar *a = FindVar(na);
            NyotaVar *b = FindVar(nb);
            uint32_t i;
            if (!a || a->val.type != TYPE_LIST || !b || b->val.type != TYPE_LIST) {
                OutError("EXTEND wymaga dwoch LIST");
                return;
            }
            for (i = 0; i < b->val.list_len; i++) {
                if (ListFindEq(&a->val, &b->val.list_items[i]) >= 0) continue;
                if (a->val.list_len >= a->val.list_cap) { OutError("EXTEND: lista pelna"); return; }
                a->val.list_items[a->val.list_len++] = b->val.list_items[i];
            }
        }
        return;
    }
''',
'''    // --- EXTEND a, b ---
    if (NStartsWith(line, "EXTEND")) {
        const char *p = NTrim(line + 6);
        char na[64], nb[64];
        uint32_t n1 = ParseIdent(p, na, sizeof(na));
        uint32_t n2;
        p = NTrim(p + n1);
        if (!na[0] || *p != ',') { OutError("EXTEND: wymagane EXTEND a, b"); return; }
        p = NTrim(p + 1);
        n2 = ParseIdent(p, nb, sizeof(nb));
        if (!nb[0] || *NTrim(p + n2)) { OutError("EXTEND: wymagane dwie nazwy LIST"); return; }
        {
            NyotaVar *a = FindVar(na);
            NyotaVar *b = FindVar(nb);
            uint32_t i, source_len;
            if (!a || a->val.type != TYPE_LIST || !b || b->val.type != TYPE_LIST) {
                OutError("EXTEND wymaga dwoch LIST");
                return;
            }
            if (a->is_const) { OutError("Nie mozna zmienic stalej LIST"); return; }
            source_len = b->val.list_len;
            for (i = 0; i < source_len; i++) {
                if (ListFindEq(&a->val, &b->val.list_items[i]) >= 0) continue;
                if (!ListEnsureCapacity(&a->val, a->val.list_len + 1)) return;
                a->val.list_items[a->val.list_len++] = b->val.list_items[i];
            }
        }
        return;
    }
''')

# REVERSE validates the whole command and honors CONST.
replace_once(src,
'''    if (NStartsWith(line, "REVERSE")) {
        const char *p = NTrim(line + 7);
        char name[64];
        ParseIdent(p, name, sizeof(name));
        {
            NyotaVar *v = FindVar(name);
            uint32_t i, n;
            if (!v || v->val.type != TYPE_LIST) { OutError("REVERSE wymaga LIST"); return; }
''',
'''    if (NStartsWith(line, "REVERSE")) {
        const char *p = NTrim(line + 7);
        char name[64];
        uint32_t nlen = ParseIdent(p, name, sizeof(name));
        if (!name[0] || *NTrim(p + nlen)) { OutError("REVERSE wymaga jednej nazwy LIST"); return; }
        {
            NyotaVar *v = FindVar(name);
            uint32_t i, n;
            if (!v || v->val.type != TYPE_LIST) { OutError("REVERSE wymaga LIST"); return; }
            if (v->is_const) { OutError("Nie mozna zmienic stalej LIST"); return; }
''')

# SORT accepts INTEGER/FLOAT/STRING/DATE only and validates trailing syntax.
replace_once(src,
'''    // --- SORT lista  /  SORT lista, DESC ---
    if (NStartsWith(line, "SORT")) {
        const char *p = NTrim(line + 4);
        char name[64];
        uint32_t nlen = ParseIdent(p, name, sizeof(name));
        NyotaVar *v = FindVar(name);
        int desc = 0;
        p = NTrim(p + nlen);
        if (*p == ',') {
            p = NTrim(p + 1);
            if (PeekWord(p, "DESC")) desc = 1;
            else if (PeekWord(p, "ASC")) desc = 0;
            else { OutError("SORT: uzyj DESC albo ASC"); return; }
        }
        if (!v || v->val.type != TYPE_LIST) { OutError("SORT wymaga LIST"); return; }
        {
            uint32_t i;
            for (i = 1; i < v->val.list_len; i++) {
                if (v->val.list_items[i].type != v->val.list_items[0].type) {
                    OutError("SORT: lista typow mieszanych");
                    return;
                }
            }
        }
        ListSort(&v->val, desc);
        return;
    }
''',
'''    // --- SORT lista  /  SORT lista, DESC ---
    if (NStartsWith(line, "SORT")) {
        const char *p = NTrim(line + 4);
        char name[64];
        uint32_t nlen = ParseIdent(p, name, sizeof(name));
        NyotaVar *v = FindVar(name);
        int desc = 0;
        p = NTrim(p + nlen);
        if (*p == ',') {
            p = NTrim(p + 1);
            if (PeekWord(p, "DESC")) { desc = 1; p = NTrim(p + 4); }
            else if (PeekWord(p, "ASC")) { desc = 0; p = NTrim(p + 3); }
            else { OutError("SORT: uzyj DESC albo ASC"); return; }
        }
        if (*p) { OutError("SORT: nadmiarowa skladnia"); return; }
        if (!v || v->val.type != TYPE_LIST) { OutError("SORT wymaga LIST"); return; }
        if (v->is_const) { OutError("Nie mozna zmienic stalej LIST"); return; }
        if (v->val.list_len > 0) {
            uint8_t t = v->val.list_items[0].type;
            uint32_t i;
            if (t != TYPE_INT && t != TYPE_FLOAT && t != TYPE_STR && t != TYPE_DATE) {
                OutError("SORT: obslugiwane typy to INTEGER, FLOAT, STRING, DATE");
                return;
            }
            for (i = 1; i < v->val.list_len; i++) {
                if (v->val.list_items[i].type != t) {
                    OutError("SORT: lista typow mieszanych");
                    return;
                }
            }
        }
        ListSort(&v->val, desc);
        return;
    }
''')

# Keep the authoritative agent spec synchronized with the implementation.
replace_once("docs/nyota.md",
'''- <span style="color: navy;">LIST: `+` `-` `><`, EXTEND, REMOVE po indeksie i wartości, CLEAR, REVERSE, SORT/DESC w toku (zaawansowany etap) 2026-09-17</span>
''',
'''- <span style="color: #006A4E;">LIST: literały (także zagnieżdżone), indeksowanie INTEGER, `+` `-` `><`, IN/LEN, APPEND/EXTEND/REMOVE/CLEAR/REVERSE/SORT, FOR...IN, RANDINT/RANDFLT wykonane 2026-09-17</span>
''')

replace_once("docs/nyota.md",
'''Operacje:

```nyota
APPEND a, "cztery"
REMOVE a[0]
REMOVE a, "trzy"
REMOVE a, 5, ALL
EXTEND a, b
REVERSE a
SORT a
SORT a, DESC
CLEAR a
PRINT LEN(a)
```

Operatory zwracają nową listę; instrukcje mutują istniejącą:

```nyota
VAR c := a + b
VAR d := a - b
VAR e := a >< b
```

Porównanie elementów jest ścisłe typowo: `5`, `"5"` i `5.0` są różne.

Zasady:

* `APPEND`, `REMOVE`, `EXTEND`, `REVERSE`, `SORT`, `CLEAR` są instrukcjami,
* `LEN(a)` jest funkcją,
* `REMOVE a[i]` usuwa po indeksie; `REMOVE a, wartosc` po wartości,
* przypisanie poza zakresem listy powoduje błąd,
* operacje na elementach list wymagają zgodnych typów.
''',
'''Operacje:

```nyota
APPEND a, "cztery"
REMOVE a[0]
REMOVE a, "trzy"
REMOVE a, 5, ALL
EXTEND a, b
REVERSE a
SORT a
SORT a, DESC
CLEAR a
PRINT LEN(a)
```

Operatory zwracają nową listę; instrukcje mutują istniejącą:

```nyota
VAR c := a + b
VAR d := a - b
VAR e := a >< b
```

Iteracja działa bezpośrednio po elementach listy:

```nyota
FOR element IN a:
    PRINT element
```

Losowe listy liczbowe:

```nyota
VAR liczby := RANDINT(10, 1, 100)
VAR pomiary := RANDFLT(10, 0, 1, 3)
```

`RANDFLT` ma domyślną precyzję 2; jawna precyzja może wynosić `0..3`.

Porównanie elementów jest ścisłe typowo: `5`, `"5"` i `5.0` są różne.

Zasady:

* `APPEND`, `REMOVE`, `EXTEND`, `REVERSE`, `SORT`, `CLEAR` są instrukcjami mutującymi,
* `CONST` zawierający LIST nie może być mutowany tymi instrukcjami ani przez indeks,
* `LEN(a)` jest funkcją, a `x IN a` używa ścisłej tożsamości typu i wartości,
* indeks LIST musi być typu `INTEGER`; nie ma automatycznej konwersji indeksu,
* `REMOVE a[i]` usuwa po indeksie; `REMOVE a, wartosc` po wartości; `ALL` usuwa wszystkie wystąpienia,
* `SORT` obsługuje jednorodne listy `INTEGER`, `FLOAT`, `STRING` i `DATE`; `ASC` jest domyślne, `DESC` odwraca kierunek,
* `+`, `-` i `><` zwracają nową listę i nie mutują operandów,
* bieżący interpreter ma limit implementacyjny 64 elementów jednej LIST; nie jest to deklarowany limit języka.
''')

# Update the v0.5 plan: the approved LIST wave is now implemented.
replace_once("docs/nyota_v05.md",
'''**Status wdrożenia:** APPROVED AFTER CORE (pierwszy wycinek wdrożony wcześniej)  
<span style="color: navy;">`+` `-` `><`, EXTEND, REMOVE indeks/wartość/ALL, CLEAR, REVERSE w toku (zaawansowany etap) 2026-09-17</span>

Operatory `+`, `-` i `><` zwracają nową listę. Instrukcje `EXTEND`, `REMOVE`,
`CLEAR`, `REVERSE` i `SORT` zmieniają istniejącą listę w miejscu.
''',
'''**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">pełna zatwierdzona powierzchnia LIST: literały i zagnieżdżenia, indeksowanie, `+` `-` `><`, IN/LEN, APPEND/EXTEND/REMOVE/CLEAR/REVERSE/SORT, FOR...IN wykonane 2026-09-17</span>

Operatory `+`, `-` i `><` zwracają nową listę. Instrukcje `APPEND`, `EXTEND`, `REMOVE`,
`CLEAR`, `REVERSE` i `SORT` zmieniają istniejącą listę w miejscu.
''')

replace_once("docs/nyota_v05.md",
'''```text
5 != "5"
5 != 5.0
5 != TRUE
```
''',
'''```text
5 <> "5"
5 <> 5.0
5 <> TRUE
```
''')

replace_once("docs/nyota_v05.md",
'''```nyota
REVERSE lista
```

## 6.10. Losowe listy liczbowe

**Status wdrożenia:** APPROVED AFTER CORE
''',
'''```nyota
REVERSE lista
```

### FOR ... IN LIST

`FOR ... IN` przechodzi po elementach listy w ich bieżącej kolejności. Wyrażenie
listy jest obliczane raz przy wejściu do pętli. Zmienna iteratora jest zmienną
sterującą pętli i przy liście mieszanej przyjmuje typ aktualnego elementu.

```nyota
FOR element IN lista:
    PRINT element
```

Indeksowanie LIST jest ścisłe: indeks musi być `INTEGER`. `"1"`, `1.0` ani
`TRUE` nie są automatycznie zamieniane na indeks całkowity.

Bieżący interpreter ma limit implementacyjny `64` elementów jednej LIST.
Limit ten wynika z obecnej statycznej implementacji i nie jest deklarowanym
limitem semantycznym języka.

## 6.10. Losowe listy liczbowe

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">RANDINT i RANDFLT, count do 64 i precyzja RANDFLT 0..3 wykonane 2026-09-17</span>
''')

# Focused regression tests. These use the stricter `zawiera` expectation mode.
tests = {
    "tests/list_append_derived.nyo": '''# Oczekiwane: zawiera 3 3\nBEGIN\nVAR a := [1] + [2]\nAPPEND a, 3\nPRINT LEN(a), a[2]\nEND\n''',
    "tests/list_nested.nyo": '''# Oczekiwane: zawiera 3\nBEGIN\nVAR a := [1, [2, 3], 4]\nPRINT a[1][1]\nEND\n''',
    "tests/list_index_type.nyo": '''# Oczekiwane: BLAD Indeks LIST wymaga INTEGER\nBEGIN\nVAR a := [10, 20]\nPRINT a["0"]\nEND\n''',
    "tests/list_remove_all.nyo": '''# Oczekiwane: zawiera 1 2\nBEGIN\nVAR a := [1, 1, 2, 1]\nREMOVE a, 1, ALL\nPRINT LEN(a), a[0]\nEND\n''',
    "tests/list_remove_string_comma.nyo": '''# Oczekiwane: zawiera 1\nBEGIN\nVAR a := ["x, ALL", "z"]\nREMOVE a, "x, ALL"\nPRINT LEN(a)\nEND\n''',
    "tests/list_sort_string.nyo": '''# Oczekiwane: zawiera a c\nBEGIN\nVAR a := ["c", "a", "b"]\nSORT a\nPRINT a[0], a[2]\nEND\n''',
    "tests/list_sort_date.nyo": '''# Oczekiwane: zawiera <2025.01.01>\nBEGIN\nVAR a := [<2026.09.17>, <2025.01.01>, <2026.01.01>]\nSORT a\nPRINT a[0]\nEND\n''',
    "tests/list_sort_bool_error.nyo": '''# Oczekiwane: BLAD SORT\nBEGIN\nVAR a := [TRUE, FALSE]\nSORT a\nEND\n''',
    "tests/list_for_in.nyo": '''# Oczekiwane: zawiera 6\nBEGIN\nVAR suma := 0\nFOR x IN [1, 2, 3]:\n    suma := suma + x\nPRINT suma\nEND\n''',
    "tests/list_in_strict.nyo": '''# Oczekiwane: zawiera STRICT\nBEGIN\nIF 5.0 IN [5]:\n    PRINT "BAD"\nELSE:\n    PRINT "STRICT"\nEND\n''',
    "tests/list_const_mut.nyo": '''# Oczekiwane: BLAD Nie mozna zmienic stalej LIST\nCONST a := [1, 2]\nBEGIN\nAPPEND a, 3\nEND\n''',
    "tests/list_randint.nyo": '''# Oczekiwane: zawiera 8\nBEGIN\nVAR a := RANDINT(8, 1, 3)\nPRINT LEN(a)\nEND\n''',
    "tests/list_randflt.nyo": '''# Oczekiwane: zawiera 7\nBEGIN\nVAR a := RANDFLT(7, -1, 1, 3)\nPRINT LEN(a)\nEND\n''',
    "tests/list_randflt_precision_error.nyo": '''# Oczekiwane: BLAD RANDFLT\nBEGIN\nVAR a := RANDFLT(2, 0, 1, 4)\nEND\n''',
}
for name, content in tests.items():
    Path(name).write_text(content, encoding="utf-8")

print("LIST upgrade prepared:", len(tests), "new regression tests")
