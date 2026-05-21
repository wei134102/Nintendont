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

#define CHEAT_LIST_MAX   10
#define CHEAT_LINE_Y0    (MENU_POS_Y + 20 * 4)
#define CHEAT_PREVIEW_Y  (MENU_POS_Y + 20 * 16)
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

static int CheatMenu_CountEnabled(const GCT_Cheats *cheats)
{
	u16 i, n = 0;
	for (i = 0; i < cheats->cheat_count; i++) {
		if (cheats->entries[i].enabled)
			n++;
	}
	return n;
}

static void CheatMenu_DrawPreview(const GCT_Cheats *cheats, s32 cursor)
{
	const GCT_CheatEntry *e;
	u32 y = CHEAT_PREVIEW_Y;

	PrintFormat(DEFAULT_SIZE, DARK_BLUE, MENU_POS_X, y,
		"-------- 当前项预览 --------");
	y += 18;

	if (cursor < 0 || cursor >= (s32)cheats->cheat_count) {
		PrintFormat(DEFAULT_SIZE, GRAY, MENU_POS_X, y, "(无)");
		return;
	}

	e = &cheats->entries[cursor];
	PrintFormat(DEFAULT_SIZE, BLACK, MENU_POS_X, y, "%.60s", e->name);
	y += 18;

	if (e->comment[0])
		PrintFormat(DEFAULT_SIZE, GRAY, MENU_POS_X, y, "%.58s", e->comment);
	y += 18;

	PrintFormat(DEFAULT_SIZE, BLACK, MENU_POS_X, y, "代码行数: %u  (每行 8+8 位)",
		(unsigned)(e->code_count / 2));

	if (e->code_count >= 2) {
		y += 18;
		PrintFormat(DEFAULT_SIZE, GRAY, MENU_POS_X, y, "首行: %08X %08X",
			e->codes[0], e->codes[1]);
	}
	if (e->code_count >= 4) {
		y += 18;
		PrintFormat(DEFAULT_SIZE, GRAY, MENU_POS_X, y, "次行: %08X %08X",
			e->codes[2], e->codes[3]);
	}
}

static void CheatMenu_Draw(const gameinfo *gi, const GCT_Cheats *cheats,
	const char *txt_path, const char *gct_path,
	s32 scroll, s32 cursor, const char *status)
{
	u32 i;
	s32 y = CHEAT_LINE_Y0;
	s32 end;
	int enabled = CheatMenu_CountEnabled(cheats);

	PrintFormat(DEFAULT_SIZE, BLACK, STR_CONST_X("金手指"), MENU_POS_Y + 20, "金手指");
	PrintFormat(DEFAULT_SIZE, BLACK, MENU_POS_X, MENU_POS_Y + 20 * 2,
		"%.6s  %s", gi->ID, cheats->game_title[0] ? cheats->game_title : gi->Name);
	PrintFormat(DEFAULT_SIZE, GRAY, MENU_POS_X, MENU_POS_Y + 20 * 3,
		"共 %u 项 | 已开 %d 项 | %u/%u", (unsigned)cheats->cheat_count, enabled,
		(unsigned)(cursor + 1), (unsigned)cheats->cheat_count);

	end = scroll + CHEAT_LIST_MAX;
	if (end > (s32)cheats->cheat_count)
		end = cheats->cheat_count;

	for (i = (u32)scroll; i < (u32)end; i++, y += 18) {
		const GCT_CheatEntry *e = &cheats->entries[i];
		u32 color = (i == (u32)cursor) ? DARK_BLUE : BLACK;
		PrintFormat(DEFAULT_SIZE, color, MENU_POS_X, y, "%s",
			(i == (u32)cursor) ? ARROW_LEFT : " ");
		PrintFormat(DEFAULT_SIZE, color, MENU_POS_X + 16, y, "%-36.36s", e->name);
		PrintFormat(DEFAULT_SIZE, color, MENU_POS_X + 380, y, "%2u行",
			(unsigned)(e->code_count / 2));
		PrintFormat(DEFAULT_SIZE, color, MENU_POS_X + 430, y, "%s",
			e->enabled ? "开" : "关");
	}

	CheatMenu_DrawPreview(cheats, cursor);

	if (status && status[0])
		PrintFormat(DEFAULT_SIZE, GREEN, MENU_POS_X, MENU_POS_Y + 20 * 19, "%s", status);

	PrintFormat(DEFAULT_SIZE, BLACK, MENU_POS_X, MENU_POS_Y + 20 * 20,
		"A:开关  X:写入GCT(覆盖)  B:全关  Home:返回");
	PrintFormat(DEFAULT_SIZE, GRAY, MENU_POS_X, MENU_POS_Y + 20 * 3 + 16,
		"GCT:%s", gct_path);
}

void CheatMenu_Show(const gameinfo *gi)
{
	char txt_path[128];
	char gct_path[128];
	char status[96];
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

	/* 默认全部关闭；若已有 GCT 则仅勾选其中包含的项（同 USB Loader GX） */
	GCT_MarkEnabledFromFile(&cheats, gct_path);

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
			int n = CheatMenu_CountEnabled(&cheats);
			if (n == 0) {
				strncpy(status, "请至少开启一项再生成 GCT", sizeof(status) - 1);
				redraw = true;
			} else if (GCT_CreateGCT(&cheats, gct_path)) {
				ncfg->Config |= NIN_CFG_CHEATS;
				ncfg->Config &= ~NIN_CFG_CHEAT_PATH;
				DCFlushRange((void *)ncfg, sizeof(NIN_CFG));
				snprintf(status, sizeof(status),
					"已覆盖写入 GCT (%d/%u 项)", n, (unsigned)cheats.cheat_count);
				redraw = true;
			} else {
				strncpy(status, "写入 codes 目录失败", sizeof(status) - 1);
				redraw = true;
			}
		}
	}
}
