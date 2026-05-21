#!/usr/bin/env python3
"""Apply Chinese strings to ShowGameInfo.c (no mysterio)."""
from pathlib import Path

official = Path(__file__).resolve().parents[1] / "loader/source/ShowGameInfo.c"
mmod = (
    Path(__file__).resolve().parents[1].parent
    / "Nintendont-MMMod-old-chs/loader/source/ShowGameInfo.c"
)
text = official.read_text(encoding="utf-8", errors="replace")
m_text = mmod.read_text(encoding="utf-8", errors="replace")

replacements = [
    ('"Title:    %s"', '"名 称 :         %s"'),
    ('"Game ID:  %.6s"', '"游戏ID:         %.6s"'),
    ('"Region:   %s"', '"区 域 :         %s"'),
    ('"Revision: %02u"', '"版 本 :         %02u"'),
    ('"Disc #:   %u"', '"碟 #  :         %u"'),
    ('"Format:"', '"格 式 :"'),
    ('"A   : Verify MD5"', '"A   : 验证 MD5"'),
    ('"B   : Back"', '"B   : 返 回"'),
    ('"Press HOME to cancel"', '"按 HOME 键取消"'),
]

# Pull PressHome constant from MMMod if present
for line in m_text.splitlines():
    if "PressHome" in line and "const char" in line:
        for oline in text.splitlines():
            if "PressHome" in oline and "const char" in oline:
                if oline != line.replace("text_color", "BLACK"):
                    text = text.replace(oline, line)
                break
        break

for old, new in replacements:
    text = text.replace(old, new)

# MD5 status strings - copy known MMMod patterns
md5_repl = [
    ("Calculating MD5...", "正在计算 MD5..."),
    ("MD5: ", "MD5: "),
    ("Verified!", "验证通过!"),
    ("Not in database.", "不在数据库中."),
    ("Mismatch!", "不匹配!"),
    ("Read error!", "读取错误!"),
    ("Cancelled.", "已取消."),
    ("Press A to verify MD5", "按 A 验证 MD5"),
    ("Press B to go back", "按 B 返回"),
]
for old, new in md5_repl:
    if old in m_text:
        text = text.replace(old, new)

official.write_text(text, encoding="utf-8", newline="\n")
print("Updated", official)
