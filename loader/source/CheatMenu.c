/* In-loader cheat menu: read txtcode .txt, write codes .gct */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ogc/video.h>

#include "CheatMenu.h"
#include "gct.h"
#include "font.h"
#include "global.h"
#include "FPad.h"
#include "ff_utf8.h"
#include "../../common/include/CommonConfig.h"

#define CHEAT_LIST_MAX   12
#define CHEAT_LINE_Y0    (MENU_POS_Y + 20 * 4)
#define STR_X(len)       ((640 - ((len) * 10)) / 2)
#define STR_CONST_X(str) STR_X(sizeof(str) - 1)

static void CheatPaths_EnsureDirs(void)
{
	char dir[32];

	snprintf(dir, sizeof(dir), "%s:/txtcode", GetRootDevice());
	f_mkdir_char(dir);

	snprintf(dir, sizeof(dir), "%s:/codes", GetRootDevice());
	f_mkdir_char(dir);
}

static void CheatPaths_Build(const gameinfo *gi, char *txt_path, char *gct_path, size_t len)
{
	snprintf(txt_path, len, "%s:/txtcode/%.6s.txt", GetRootDevice(), gi->ID);
	snprintf(gct_path, len, "%s:/codes/%.6s.gct", GetRootDevice(), gi->ID);
}

static void CheatMenu_Draw(const gameinfo *gi, const GCT_Cheats *cheats,
	const char *txt_path, const char *gct_path,
	s32 scroll, s32 cursor, const char *status)
{
	u32 i;
	s32 y = CHEAT_LINE_Y0;
	s32 end;

	PrintFormat(DEFAULT_SIZE, BLACK, STR_CONST_X("金手指"), MENU_POS_Y + 20, "金手指");
	PrintFormat(DEFAULT_SIZE, BLACK, MENU_POS_X, MENU_POS_Y + 20 * 2,
		"%.6s  %s", gi->ID, cheats->game_title[0] ? cheats->game_title : gi->Name);
	PrintFormat(DEFAULT_SIZE, GRAY, MENU_POS_X, MENU_POS_Y + 20 * 3,
		"TXT: %s", txt_path);
	PrintFormat(DEFAULT_SIZE, GRAY, MENU_POS_X, MENU_POS_Y + 20 * 3 + 16,
		"GCT: %s", gct_path);

	end = scroll + CHEAT_LIST_MAX;
	if (end > (s32)cheats->cheat_count)
		end = cheats->cheat_count;

	for (i = (u32)scroll; i < (u32)end; i++, y += 20) {
		const GCT_CheatEntry *e = &cheats->entries[i];
		u32 color = (i == (u32)cursor) ? DARK_BLUE : BLACK;
		PrintFormat(DEFAULT_SIZE, color, MENU_POS_X, y, "%s",
			(i == (u32)cursor) ? ARROW_LEFT : " ");
		PrintFormat(DEFAULT_SIZE, color, MENU_POS_X + 20, y, "%-42.42s", e->name);
		PrintFormat(DEFAULT_SIZE, color, MENU_POS_X + 450, y, "%s",
			e->enabled ? "开" : "关");
	}

	if (status && status[0])
		PrintFormat(DEFAULT_SIZE, GREEN, MENU_POS_X, MENU_POS_Y + 20 * 18, "%s", status);

	PrintFormat(DEFAULT_SIZE, BLACK, MENU_POS_X, MENU_POS_Y + 20 * 19,
		"A:开关  X:生成GCT  B:全关  Home:返回");
}

static int CheatMenu_CountEnabled(const GCT_Cheats *cheats)
{
	u16 i, n = 0;
	for (i = 0; i < cheats->cheat_count; i++) {
		if (cheats->entries[i].enabled)
			n++;
	}
	return n;
}

void CheatMenu_Show(const gameinfo *gi)
{
	char txt_path[128];
	char gct_path[128];
	char status[80];
	GCT_Cheats cheats;
	s32 scroll = 0;
	s32 cursor = 0;
	int load_res;
	bool redraw = true;
	bool done = false;

	memset(status, 0, sizeof(status));
	CheatPaths_EnsureDirs();
	CheatPaths_Build(gi, txt_path, gct_path, sizeof(txt_path));

	load_res = GCT_OpenTxtFile(txt_path, &cheats);
	if (load_res == 0) {
		ShowMessageScreen("未找到 TXT 码表\n请将码表放到 txtcode 目录\n例如:\ntxtcode/XXXXXX.txt");
		return;
	}
	if (load_res < 0) {
		ShowMessageScreen("TXT 码表为空或格式无效");
		return;
	}

	GCT_MarkEnabledFromFile(&cheats, gct_path);
	if (CheatMenu_CountEnabled(&cheats) == 0 && cheats.cheat_count > 0)
		cheats.entries[0].enabled = 1;

	while (!done) {
		if (redraw) {
			CheatMenu_Draw(gi, &cheats, txt_path, gct_path, scroll, cursor, status);
			GRRLIB_Render();
			ClearScreen();
			redraw = false;
		}

		VIDEO_WaitVSync();
		FPAD_Update();

		if (FPAD_Start(0)) {
			done = true;
			continue;
		}

		if (FPAD_Cancel(0)) {
			u16 i;
			for (i = 0; i < cheats.cheat_count; i++)
				cheats.entries[i].enabled = 0;
			status[0] = 0;
			redraw = true;
			continue;
		}

		if (FPAD_Up(0)) {
			if (cursor > 0) {
				cursor--;
				if (cursor < scroll)
					scroll = cursor;
			} else if (cheats.cheat_count > 0) {
				cursor = cheats.cheat_count - 1;
				scroll = cursor - CHEAT_LIST_MAX + 1;
				if (scroll < 0)
					scroll = 0;
			}
			redraw = true;
		}

		if (FPAD_Down(0)) {
			if (cursor + 1 < (s32)cheats.cheat_count) {
				cursor++;
				if (cursor >= scroll + CHEAT_LIST_MAX)
					scroll = cursor - CHEAT_LIST_MAX + 1;
			} else {
				cursor = 0;
				scroll = 0;
			}
			redraw = true;
		}

		if (FPAD_OK(0) && cheats.cheat_count > 0) {
			cheats.entries[cursor].enabled = !cheats.entries[cursor].enabled;
			status[0] = 0;
			redraw = true;
		}

		if (FPAD_X(0)) {
			if (CheatMenu_CountEnabled(&cheats) == 0) {
				strncpy(status, "请至少开启一项作弊", sizeof(status) - 1);
				redraw = true;
			} else if (GCT_CreateGCT(&cheats, gct_path)) {
				ncfg->Config |= NIN_CFG_CHEATS;
				ncfg->Config &= ~NIN_CFG_CHEAT_PATH;
				DCFlushRange((void *)ncfg, sizeof(NIN_CFG));
				strncpy(status, "已生成 GCT，已打开「作弊码」", sizeof(status) - 1);
				redraw = true;
			} else {
				strncpy(status, "写入 GCT 失败", sizeof(status) - 1);
				redraw = true;
			}
		}
	}
}
