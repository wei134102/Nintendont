#!/usr/bin/env python3
"""Apply Chinese menu strings from MMMod to official Nintendont menu.c."""
from pathlib import Path

official = Path(__file__).resolve().parents[1] / "loader/source/menu.c"
mmod = Path(__file__).resolve().parents[1].parent / "Nintendont-MMMod-old-chs/loader/source/menu.c"
text = official.read_text(encoding="utf-8", errors="replace")

o_start = text.index("static const char *const *GetSettingsDescription")
o_end = text.index("static bool UpdateSettingsMenu", o_start)
m_text = mmod.read_text(encoding="utf-8", errors="replace")
m_start = m_text.index("static const char *const *GetSettingsDescription")
m_end = m_text.index("static bool UpdateSettingsMenu", m_start)
text = text[:o_start] + m_text[m_start:m_end] + text[o_end:]
text = text.replace("text_color", "BLACK")

replacements = [
    ("Multi-Game Disc (%s)", "合集游戏 (%s)"),
    ('"Boot included GC Disc"', '"从光驱中读取游戏"'),
    ('"Boot GC Disc in Drive"', '"从光驱中启动GC游戏"'),
    ('"On "', '"打开"'),
    ('"Off"', '"关闭"'),
    ('"Yes"', '"是"'),
    ('"No "', '"否"'),
    ('"Unstable"', '"不稳定"'),
    (
        'PrintFormat(MENU_SIZE, BLACK, MENU_POS_X+300, SettingY(0), "Auto");',
        'PrintFormat(MENU_SIZE, BLACK, MENU_POS_X+300, SettingY(0), "自动");',
    ),
    ('PrintButtonActions("Go Back"', 'PrintButtonActions("返 回"'),
    ('PrintButtonActions("Exit"', 'PrintButtonActions("退 出"'),
    ('"Go Back"', '"返 回"'),
    ('"Select"', '"选 择"'),
    ('"Settings"', '"设 置"'),
    ('"Update"', '"更 新"'),
    ('"A   : Select"', '"A   : 选 择"'),
    ('"X/1 : Game Info"', '"X/1 : 游戏信息"'),
    ('"Built   : "', '"编译日期: "'),
    ('"Firmware: "', '"固件    : "'),
    ('"Returning to loader..."', '"返回到loader..."'),
    (
        '"WARNING: %s FAT device could not be opened."',
        '"警告: 无法读取%s FAT设备"',
    ),
    (
        '"WARNING: %s:/games/ was not found."',
        '"警告: 目录不存在： %s:/games/ "',
    ),
    (
        '"WARNING: %s:/games/ contains no GC titles."',
        '"警告: 目录中不存在游戏： %s:/games/ "',
    ),
    ('"Failed to load IOS58 from NAND:"', '"加载IOS58失败:"'),
    (
        '"LoadKernel() error %d occurred, returning %d."',
        '"加载核心方法中发生 %d 错误, 返回 %d."',
    ),
    ('"ES_GetStoredTMDSize() returned %d."', '"ES_GetStoredTMDSize() 返回了 %d."'),
    ('"This usually means IOS58 is not installed."', '"这说明IOS58没有被安装."'),
    (
        '"WARNING: On Wii U, a missing IOS58 may indicate"',
        '"警告: 表明在Wii U上缺少IOS58"',
    ),
    (
        '"something is seriously wrong with the vWii setup."',
        '"vWii的设置中，可能存在严重错误."',
    ),
    (
        '"Please update to Wii System 4.3 and try running"',
        '"请更新Wii的系统版本至4.3，"',
    ),
    ('"Nintendont again."', '"然后再次运行Nintendont."'),
    (
        '"Unable to allocate memory for the IOS58 TMD."',
        '"无法从IOS58 TMD中分配内存."',
    ),
    ('"ES_GetStoredTMD() returned %d."', '"ES_GetStoredTMD() 返回了 %d."'),
    ('"WARNING: IOS58 may be corrupted."', '"警告: IOS58可能坏掉了."'),
    (
        '"IOS_Open(\\"/shared1/content.map\\") returned %d."',
        '"IOS_Open(\\"/shared1/content.map\\") 返回了 %d."',
    ),
    (
        '"This usually means Nintendont was not started with"',
        '"通常情况下说明，Nintendont没有被赋予"',
    ),
    ('"AHB access permissions."', '"从AHB启动的权限."'),
    (
        '"Please ensure that meta.xml is present"',
        '"请确保meta.xml存在"',
    ),
    (
        '"in the Nintendont directory and that it has"',
        '"在Nintendont目录中并且具有"',
    ),
    (
        '"the correct AHB access permissions."',
        '"正确的AHB访问权限."',
    ),
    ('"IOS_Open(IOS58 kernel) returned %d."', '"IOS_Open(IOS58核心) 返回了 %d."'),
    ('"IOS_Read(IOS58 kernel) returned %d."', '"IOS_Read(IOS58核心) 返回了 %d."'),
    ('"Video Width"', '"显示宽度          "'),
    ('"Screen Position"', '"屏幕位置          "'),
    ('"Patch PAL50"', '"PAL50补丁         "'),
    ('"TRI Arcade Mode"', '"TRI街机模式       "'),
    ('"Wiimote CC Rumble"', '"Wiimote控制器 震动"'),
    ('"Skip IPL"', '"跳过IPL           "'),
    ('"Network Profile"', '"网络配置文件      "'),
    ('"Over"', '"超大"'),
]

for old, new in replacements:
    text = text.replace(old, new)

official.write_text(text, encoding="utf-8", newline="\n")
print("Updated", official)
