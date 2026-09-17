from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"{path}: expected exactly one match, got {n}\nANCHOR:\n{old[:320]}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# ---------------- src/nyota.c: MARK helpers ----------------
replace_once(
    "src/nyota.c",
    "static int32_t TruncMilli(int32_t milli, int n) {\n",
    r'''// ============================================================
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
    int64_t ip = milli / 1000LL;
    int64_t frac = milli % 1000LL;
    if (ip < INT32_MIN || ip > INT32_MAX) {
        OutError("Wynik FLOAT poza zakresem implementacji");
        ValClear(v);
        return;
    }
    ValFromFloat(v, (int32_t)ip, (int32_t)frac);
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
''',
)

# MARK literal: reject duplicate keys.
replace_once(
    "src/nyota.c",
    "                    if (result.list_len < result.list_cap)\n                        result.list_items[result.list_len++] = row;\n",
    "                    if (FindMarkRow(&result, &row.list_items[0]) >= 0) {\n                        OutError(\"MARK: duplikat klucza\");\n                        ValClear(&result);\n                        *pp = expr;\n                        return result;\n                    }\n                    if (result.list_len < result.list_cap)\n                        result.list_items[result.list_len++] = row;\n",
)

# Algebra MARK.
replace_once(
    "src/nyota.c",
    "    if (a->type == TYPE_LIST) {\n        if (op == '+') return ListConcat(a, b);\n        if (op == '-') return ListSubtract(a, b);\n        OutError(\"LIST obsluguje tylko + i -\");\n        return r;\n    }\n",
    "    if (a->type == TYPE_LIST) {\n        if (op == '+') return ListConcat(a, b);\n        if (op == '-') return ListSubtract(a, b);\n        OutError(\"LIST obsluguje tylko + i -\");\n        return r;\n    }\n    if (a->type == TYPE_MARK) {\n        if (op == '+') return MarkConcat(a, b);\n        if (op == '-') return MarkSubtract(a, b);\n        OutError(\"MARK obsluguje tylko + i - w arytmetyce\");\n        return r;\n    }\n",
)
replace_once(
    "src/nyota.c",
    "        if (NStrEq(op, \"><\") && left.type == TYPE_LIST && right.type == TYPE_LIST)\n            return ListSymDiff(&left, &right);\n        return ValCompareOp(&left, op, &right);\n",
    "        if (NStrEq(op, \"><\") && left.type == TYPE_LIST && right.type == TYPE_LIST)\n            return ListSymDiff(&left, &right);\n        if (NStrEq(op, \"><\") && left.type == TYPE_MARK && right.type == TYPE_MARK)\n            return MarkSymDiff(&left, &right);\n        return ValCompareOp(&left, op, &right);\n",
)

# KEY/VALUE strict indexes + VALUES/MLIST/stats.
old_key_value = r'''    if (NStrEqN(expr, "KEY(", 4)) {
        char args[2][MAX_STR_LEN];
        int n = SplitFunctionArgs(expr + 4, args, 2);
        if (n != 2) { OutError("KEY(mark, wiersz)"); ValClear(&result); *pp = MatchParen(call_open); return result; }
        {
            NyotaVal mk = Eval(args[0]);
            NyotaVal ix = Eval(args[1]);
            int32_t i = ValToInt(&ix);
            if (mk.type != TYPE_MARK) { OutError("KEY() wymaga MARK"); }
            else if (i < 0 || (uint32_t)i >= mk.list_len) OutError("KEY: wiersz poza zakresem");
            else if (mk.list_items[i].type == TYPE_LIST && mk.list_items[i].list_len > 0)
                result = mk.list_items[i].list_items[0];
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
            int32_t r = ValToInt(&rv);
            int32_t c = ValToInt(&cv);
            if (mk.type != TYPE_MARK) OutError("VALUE() wymaga MARK");
            else if (r < 0 || (uint32_t)r >= mk.list_len) OutError("VALUE: wiersz poza zakresem");
            else if (c < 0 || c >= mk.i) OutError("VALUE: kolumna poza zakresem");
            else if (mk.list_items[r].type == TYPE_LIST
                     && (uint32_t)(c + 1) < mk.list_items[r].list_len)
                result = mk.list_items[r].list_items[c + 1];
        }
        *pp = call_open ? MatchParen(call_open) : expr;
        return result;
    }
'''
new_key_value = r'''    if (NStrEqN(expr, "KEY(", 4)) {
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
'''
replace_once("src/nyota.c", old_key_value, new_key_value)

# CONST MARK protections and strict cell column index.
replace_once(
    "src/nyota.c",
    "                if (!v) { OutError(\"Zmienna niezadeklarowana\"); return; }\n                if (v->val.type == TYPE_TUPLE) {\n",
    "                if (!v) { OutError(\"Zmienna niezadeklarowana\"); return; }\n                if (v->val.type == TYPE_MARK && v->is_const) { OutError(\"Nie mozna zmienic stalej MARK\"); return; }\n                if (v->val.type == TYPE_TUPLE) {\n",
)
replace_once(
    "src/nyota.c",
    "                    int r = FindMarkRow(&v->val, &key);\n                    int32_t c = ValToInt(&colv);\n                    if (r < 0) OutError(\"Brak klucza w MARK\");\n                    else if (c < 0 || c >= v->val.i) OutError(\"Kolumna MARK poza zakresem\");\n                    else v->val.list_items[r].list_items[c + 1] = rhs;\n",
    "                    int r = FindMarkRow(&v->val, &key);\n                    int32_t c;\n                    if (colv.type != TYPE_INT) { OutError(\"Kolumna MARK wymaga INTEGER\"); return; }\n                    c = colv.i;\n                    if (r < 0) OutError(\"Brak klucza w MARK\");\n                    else if (c < 0 || c >= v->val.i) OutError(\"Kolumna MARK poza zakresem\");\n                    else v->val.list_items[r].list_items[c + 1] = rhs;\n",
)

# DELETE const protection and REKEY command.
replace_once(
    "src/nyota.c",
    "                if (!v || v->val.type != TYPE_MARK) { OutError(\"DELETE dziala tylko na MARK\"); return; }\n                r = FindMarkRow(&v->val, &key);\n",
    "                if (!v || v->val.type != TYPE_MARK) { OutError(\"DELETE dziala tylko na MARK\"); return; }\n                if (v->is_const) { OutError(\"Nie mozna zmienic stalej MARK\"); return; }\n                r = FindMarkRow(&v->val, &key);\n",
)
replace_once(
    "src/nyota.c",
    "    // --- IF / ELIF / ELSE jako jeden łańcuch ---\n",
    r'''    // --- REKEY mark, stary_klucz, nowy_klucz ---
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
''',
)

# FOR ... IN adds MARK keys.
replace_once(
    "src/nyota.c",
    "            seq = Eval(seq_expr);\n            if (seq.type != TYPE_LIST && seq.type != TYPE_TUPLE) { OutError(\"FOR ... IN wymaga LIST albo TUPLE\"); return; }\n",
    "            seq = Eval(seq_expr);\n            if (seq.type != TYPE_LIST && seq.type != TYPE_TUPLE && seq.type != TYPE_MARK) { OutError(\"FOR ... IN wymaga LIST, TUPLE albo MARK\"); return; }\n",
)
replace_once(
    "src/nyota.c",
    "            for (li = 0; li < seq.list_len; li++) {\n                v->val = seq.list_items[li];\n",
    "            for (li = 0; li < seq.list_len; li++) {\n                if (seq.type == TYPE_MARK) v->val = seq.list_items[li].list_items[0];\n                else v->val = seq.list_items[li];\n",
)

# Insert column mutation commands before APPEND.
replace_once(
    "src/nyota.c",
    "    // --- APPEND lista, element ---\n",
    r'''    // --- MEXTEND mark, count, default | defaults... ---
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
''',
)

# EXTEND supports LIST and MARK.
old_extend = r'''    // --- EXTEND a, b ---
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
'''
new_extend = r'''    // --- EXTEND a, b (LIST albo MARK) ---
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
'''
replace_once("src/nyota.c", old_extend, new_extend)

# REVERSE supports MARK.
old_reverse = r'''    // --- REVERSE lista ---
    if (NStartsWith(line, "REVERSE")) {
        const char *p = NTrim(line + 7);
        char name[64];
        uint32_t nlen = ParseIdent(p, name, sizeof(name));
        if (!name[0] || *NTrim(p + nlen)) { OutError("REVERSE wymaga jednej nazwy LIST"); return; }
        {
            NyotaVar *v = FindVar(name);
            uint32_t i, n;
            if (!v || v->val.type != TYPE_LIST) { OutError("REVERSE wymaga LIST"); return; }
            if (v->is_const) { OutError("Nie mozna zmienic stalej LIST"); return; }
            n = v->val.list_len;
            for (i = 0; i < n / 2; i++) {
                NyotaVal tmp = v->val.list_items[i];
                v->val.list_items[i] = v->val.list_items[n - 1 - i];
                v->val.list_items[n - 1 - i] = tmp;
            }
        }
        return;
    }
'''
new_reverse = r'''    // --- REVERSE LIST/MARK ---
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
'''
replace_once("src/nyota.c", old_reverse, new_reverse)

# SORT supports MARK KEY/VALUE.
old_sort_start = "    // --- SORT lista  /  SORT lista, DESC ---\n    if (NStartsWith(line, \"SORT\")) {\n"
start = Path("src/nyota.c").read_text(encoding="utf-8").find(old_sort_start)
if start < 0:
    raise SystemExit("SORT block start missing")
text = Path("src/nyota.c").read_text(encoding="utf-8")
end_marker = "\n    // --- DELAY ms ---\n"
end = text.find(end_marker, start)
if end < 0:
    raise SystemExit("SORT block end missing")
new_sort = r'''    // --- SORT LIST / SORT MARK ---
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
            int desc = 0;
            if (*p == ',') {
                p = NTrim(p + 1);
                if (PeekWord(p, "DESC")) { desc = 1; p = NTrim(p + 4); }
                else if (PeekWord(p, "ASC")) { desc = 0; p = NTrim(p + 3); }
                else { OutError("SORT: uzyj DESC albo ASC"); return; }
            }
            if (*p) { OutError("SORT: nadmiarowa skladnia"); return; }
            if (v->val.list_len > 0) {
                uint8_t t = v->val.list_items[0].type;
                uint32_t i;
                if (t != TYPE_INT && t != TYPE_FLOAT && t != TYPE_STR && t != TYPE_DATE && t != TYPE_TIME) {
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
            ListSort(&v->val, desc);
            return;
        }
        OutError("SORT wymaga LIST albo MARK");
        return;
    }
'''
Path("src/nyota.c").write_text(text[:start] + new_sort + text[end:], encoding="utf-8")

# ---------------- docs/nyota.md ----------------
replace_once(
    "docs/nyota.md",
    "- <span style=\"color: navy;\">MARK: literał, odczyt, zapis wiersza/komórki, DELETE, LEN, IN, KEY/VALUE/MINFO w toku (zaawansowany etap) 2026-09-17</span>\n",
    "- <span style=\"color: #006A4E;\">MARK: literał i mutacje, REKEY, KEY/VALUE/VALUES/MLIST/MINFO, IN/FOR IN, algebra + - ><, EXTEND/CLEAR/REVERSE/SORT, kolumny MEXTEND/MINSERT/MDROP oraz statystyki wykonane 2026-09-17</span>\n",
)
old_mark_status = '''<span style="color: navy;">literał, odczyt `m[k][c]`, zapis wiersza i komórki, DELETE, LEN, IN, KEY/VALUE/MINFO w toku (zaawansowany etap) 2026-09-17</span>  
`REKEY`, `VALUES` z zakresem, `CLEAR`, algebra `+`/`EXTEND` i statystyki jeszcze nie.
'''
new_mark_status = '''<span style="color: #006A4E;">pełny podstawowy MARK: odczyt/mutacje, algebra, iteracja, sortowanie, operacje kolumnowe i statystyki wykonane 2026-09-17</span>
'''
replace_once("docs/nyota.md", old_mark_status, new_mark_status)
mark_rules_anchor = '''* `DELETE` działa wyłącznie na MARK,
* nie mylić `DELETE` z `REMOVE`.
'''
mark_rules_more = '''* `DELETE` działa wyłącznie na MARK,
* nie mylić `DELETE` z `REMOVE`,
* `REKEY m, old, new` zmienia klucz bez zmiany pozycji wiersza,
* `FOR key IN m:` iteruje po kluczach w bieżącej kolejności,
* `+`, `-` i `><` zwracają nowy MARK; `EXTEND`, `REVERSE`, `SORT`, `CLEAR`, `MEXTEND`, `MINSERT`, `MDROP` mutują,
* `VALUES(m, row, ...)` zwraca LIST wybranych kolumn; zakres `2:7` jest domknięty,
* `MLIST(m, KEY)` zwraca klucze, a `MLIST(m, n)` wskazaną kolumnę wartości,
* `SORT m, KEY[, DESC]` oraz `SORT m, VALUE n[, DESC]` sortują wiersze,
* `SUM`, `AVG`, `MED`, `MIN`, `MAX`, `MODE`, `MODECOUNT`, `COUNT` działają kolumnowo,
* przy remisie `MODE` wybiera pierwszą wartość w aktualnej kolejności MARK-a; `MODECOUNT` zwraca jej liczność,
* indeksy wierszy/kolumn dla funkcji MARK wymagają `INTEGER`, bez konwersji niejawnej,
* `CONST` zawierający MARK nie może być mutowany.
'''
replace_once("docs/nyota.md", mark_rules_anchor, mark_rules_more)

# ---------------- docs/nyota_v05.md ----------------
p = Path("docs/nyota_v05.md")
text = p.read_text(encoding="utf-8")
text = text.replace(
    '**Status wdrożenia:** APPROVED AFTER CORE (pierwszy wycinek wdrożony wcześniej)  \n<span style="color: navy;">literał, odczyt, zapis wiersza/komórki, DELETE, LEN, IN, KEY/VALUE/MINFO w toku (zaawansowany etap) 2026-09-17</span>  \n<span style="color: yellow;">REKEY, VALUES z zakresem, CLEAR, algebra, statystyki, SORT zaczęte 2026-09-17</span>',
    '**Status wdrożenia:** APPROVED AFTER CORE — WDROŻONE  \n<span style="color: #006A4E;">REKEY, KEY/VALUE/VALUES, IN/FOR IN, algebra + - ><, EXTEND/CLEAR/REVERSE/SORT, MLIST/MEXTEND/MINSERT/MDROP i statystyki wykonane 2026-09-17</span>'
)
text = text.replace(
    'Przypadek wielu równie częstych dominant wymaga jeszcze ostatecznego\nzdefiniowania w specyfikacji.',
    'Przy remisie kilku dominant `MODE()` zwraca tę, która występuje najwcześniej\nw aktualnej kolejności MARK-a. `MODECOUNT()` zwraca liczbę jej wystąpień.'
)
p.write_text(text, encoding="utf-8")

# ---------------- README.md ----------------
replace_once(
    "README.md",
    "| `MARK` | 🚧 | autorska struktura tabelowa Nyoty |\n",
    "| `MARK` | ✅ | klucze+kolumny, algebra, iteracja, sortowanie, przebudowa kolumn i statystyki |\n",
)

# ---------------- tests ----------------
tests = {
"tests/mark_rekey.nyo": '''# Oczekiwane: zawiera 10\nBEGIN\nVAR m := {2| "A" | 10, "B" | 20}\nREKEY m, "A", "C"\nPRINT m["C"][0]\nEND\n''',
"tests/mark_rekey_collision.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR m := {2| "A" | 10, "B" | 20}\nREKEY m, "A", "B"\nEND\n''',
"tests/mark_values.nyo": '''# Oczekiwane: zawiera [20, 30, 40]\nBEGIN\nVAR m := {5| "A" | 10 | 20 | 30 | 40}\nPRINT VALUES(m, 0, 1:3)\nEND\n''',
"tests/mark_mlist.nyo": '''# Oczekiwane: zawiera [A, B]\nBEGIN\nVAR m := {3| "A" | 10 | 11, "B" | 20 | 21}\nPRINT MLIST(m, KEY)\nEND\n''',
"tests/mark_for_in.nyo": '''# Oczekiwane: zawiera A\nBEGIN\nVAR m := {2| "A" | 10, "B" | 20}\nFOR k IN m:\n    PRINT k\nEND\n''',
"tests/mark_ops.nyo": '''# Oczekiwane: zawiera 3 1 2\nBEGIN\nVAR a := {2| "A" | 1, "B" | 2}\nVAR b := {2| "C" | 3}\nVAR c := a + b\nVAR d := c - a\nVAR e := a >< b\nPRINT LEN(c), LEN(d), LEN(e)\nEND\n''',
"tests/mark_plus_conflict.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR a := {2| "A" | 1}\nVAR b := {2| "A" | 2}\nVAR c := a + b\nEND\n''',
"tests/mark_extend.nyo": '''# Oczekiwane: zawiera 3\nBEGIN\nVAR a := {2| "A" | 1, "B" | 2}\nVAR b := {2| "B" | 9, "C" | 3}\nEXTEND a, b\nPRINT LEN(a)\nEND\n''',
"tests/mark_reverse.nyo": '''# Oczekiwane: zawiera C\nBEGIN\nVAR m := {2| "A" | 1, "B" | 2, "C" | 3}\nREVERSE m\nPRINT KEY(m, 0)\nEND\n''',
"tests/mark_sort_key.nyo": '''# Oczekiwane: zawiera A\nBEGIN\nVAR m := {2| "C" | 3, "A" | 1, "B" | 2}\nSORT m, KEY\nPRINT KEY(m, 0)\nEND\n''',
"tests/mark_sort_value.nyo": '''# Oczekiwane: zawiera B\nBEGIN\nVAR m := {2| "A" | 30, "B" | 10, "C" | 20}\nSORT m, VALUE 0\nPRINT KEY(m, 0)\nEND\n''',
"tests/mark_mextend.nyo": '''# Oczekiwane: zawiera [2, 3]\nBEGIN\nVAR m := {2| "A" | 10, "B" | 20}\nMEXTEND m, 2, 0\nPRINT MINFO(m)\nEND\n''',
"tests/mark_mextend_defaults.nyo": '''# Oczekiwane: zawiera [10, X, TRUE]\nBEGIN\nVAR m := {2| "A" | 10}\nMEXTEND m, 2, "X", TRUE\nPRINT m["A"]\nEND\n''',
"tests/mark_minsert.nyo": '''# Oczekiwane: zawiera [10, X, 20]\nBEGIN\nVAR m := {3| "A" | 10 | 20}\nMINSERT m, 1, "X"\nPRINT m["A"]\nEND\n''',
"tests/mark_mdrop.nyo": '''# Oczekiwane: zawiera [10, 30]\nBEGIN\nVAR m := {4| "A" | 10 | 20 | 30}\nMDROP m, 1\nPRINT m["A"]\nEND\n''',
"tests/mark_stats.nyo": '''# Oczekiwane: zawiera 6 2.000 2 1 3 1\nBEGIN\nVAR m := {2| "A" | 1, "B" | 2, "C" | 3}\nPRINT SUM(m, 0), AVG(m, 0), MED(m, 0), MIN(m, 0), MAX(m, 0), COUNT(m, 0, 2)\nEND\n''',
"tests/mark_mode.nyo": '''# Oczekiwane: zawiera A 2\nBEGIN\nVAR m := {2| 1 | "A", 2 | "B", 3 | "B", 4 | "A"}\nPRINT MODE(m, 0), MODECOUNT(m, 0)\nEND\n''',
"tests/mark_const_mut.nyo": '''# Oczekiwane: BLAD\nBEGIN\nCONST m := {2| "A" | 1}\nREKEY m, "A", "B"\nEND\n''',
"tests/mark_key_index_type_error.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR m := {2| "A" | 1}\nPRINT KEY(m, 0.0)\nEND\n''',
"tests/mark_value_index_type_error.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR m := {2| "A" | 1}\nPRINT VALUE(m, 0, 0.0)\nEND\n''',
"tests/mark_cell_index_type_error.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR m := {2| "A" | 1}\nm["A"][0.0] := 2\nEND\n''',
"tests/mark_duplicate_key_error.nyo": '''# Oczekiwane: BLAD\nBEGIN\nVAR m := {2| "A" | 1, "A" | 2}\nEND\n''',
"tests/mark_clear_shape.nyo": '''# Oczekiwane: zawiera [0, 2]\nBEGIN\nVAR m := {3| "A" | 1 | 2}\nCLEAR m\nPRINT MINFO(m)\nEND\n''',
"tests/mark_sort_date.nyo": '''# Oczekiwane: zawiera B\nBEGIN\nVAR m := {2| "A" | <2027.01.01>, "B" | <2026.01.01>}\nSORT m, VALUE 0\nPRINT KEY(m, 0)\nEND\n''',
}
for path, content in tests.items():
    Path(path).write_text(content, encoding="utf-8")

print("MARK upgrade applied")
