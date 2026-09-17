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

p.write_text(text, encoding="utf-8")
print("made MatchParen replacement deterministic")
