from pathlib import Path

p = Path("scripts/tmp_list_upgrade.py")
text = p.read_text(encoding="utf-8")
old = '''def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
'''
new = '''def replace_once(path, old, new):
    # Python interprets \\0 inside the embedded C snippets as a NUL byte.
    # Normalize it back to the two source characters expected in nyota.c.
    old = old.replace(chr(0), "\\\\0")
    new = new.replace(chr(0), "\\\\0")
    p = Path(path)
    text = p.read_text(encoding="utf-8")
'''
if text.count(old) != 1:
    raise SystemExit("cannot patch replace_once helper")
p.write_text(text.replace(old, new, 1), encoding="utf-8")
