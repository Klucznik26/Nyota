from pathlib import Path

p = Path("scripts/tmp_tuple_time_upgrade.py")
text = p.read_text(encoding="utf-8")
text = text.replace("\\0", "\\\\0")
p.write_text(text, encoding="utf-8")
print("normalized C \\0 escapes in temporary upgrade script")
