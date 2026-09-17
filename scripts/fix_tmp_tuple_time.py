from pathlib import Path

p = Path("scripts/tmp_tuple_time_upgrade.py")
text = p.read_text(encoding="utf-8")

helper_anchor = '''\ndef insert_before(path, anchor, text_to_insert):\n'''
helper = '''\ndef replace_first(path, old, new):\n    p = Path(path)\n    text = p.read_text(encoding="utf-8")\n    if old not in text:\n        raise SystemExit(f"{path}: replacement anchor not found\\n--- OLD ---\\n{old[:500]}")\n    p.write_text(text.replace(old, new, 1), encoding="utf-8")\n\n'''
if "def replace_first(" not in text:
    if helper_anchor not in text:
        raise SystemExit("cannot insert replace_first helper")
    text = text.replace(helper_anchor, helper + helper_anchor, 1)

needle = '''replace_once(src,\n''' + "'''" + '''        else if (!in_str && (*q == '(' || *q == '[')) depth++;\n        else if (!in_str && (*q == ')' || *q == ']')) {\n''' + "'''" + ''',\n'''
replacement = '''replace_first(src,\n''' + "'''" + '''        else if (!in_str && (*q == '(' || *q == '[')) depth++;\n        else if (!in_str && (*q == ')' || *q == ']')) {\n''' + "'''" + ''',\n'''
if needle not in text:
    raise SystemExit("ambiguous MatchParen replacement call not found")
text = text.replace(needle, replacement, 1)

# The inserted helper block must end before the real ParsePrimary declaration.
dup_tail = '''    *consumed = i;\n    return 1;\n}\n\n// Prosta ewaluacja wyrażenia (bez rekurencji dla nawiasów — linearny parser)\nstatic NyotaVal ParsePrimary(const char **pp) {\n''')\n'''
fixed_tail = '''    *consumed = i;\n    return 1;\n}\n\n''')\n'''
if dup_tail not in text:
    raise SystemExit("duplicate ParsePrimary tail in temporary upgrade script not found")
text = text.replace(dup_tail, fixed_tail, 1)

# LIST was completed after the original temporary patch was prepared.
# Make the agent-facing documentation anchor follow the current green status line.
lines = text.splitlines(keepends=True)
replaced = False
for i, line in enumerate(lines):
    if line.startswith("needle = '- <span style=") and "LIST:" in line and "LIST status anchor" not in line:
        lines[i] = 'needle = \'- <span style="color: #006A4E;">LIST: literały (także zagnieżdżone), indeksowanie INTEGER, `+` `-` `><`, IN/LEN, APPEND/EXTEND/REMOVE/CLEAR/REVERSE/SORT, FOR...IN, RANDINT/RANDFLT wykonane 2026-09-17</span>\\n\'\n'
        replaced = True
        break
if not replaced:
    raise SystemExit("LIST documentation needle in temporary upgrade script not found")
text = ''.join(lines)

# Python string literals in the temporary patch must emit C '\\0', not a literal NUL byte.
text = text.replace("\\0", "\\\\0")

p.write_text(text, encoding="utf-8")
print("repaired TUPLE/TIME temporary patch: parser block, LIST docs and C NUL escapes")
