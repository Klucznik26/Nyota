from pathlib import Path

p = Path("scripts/apply_button_widget.py")
text = p.read_text(encoding="utf-8")
old = 'anchor = "## 6.15. Kierunki FUTURE\\n"'
new = 'anchor = "## 6.16. Kierunki FUTURE\\n"'
if old not in text:
    raise SystemExit("BUTTON script anchor to fix not found")
p.write_text(text.replace(old, new, 1), encoding="utf-8")
print("BUTTON docs anchor fixed")
