/*
 * Gecko TXT cheat parser and GCT writer (ported from USB Loader GX gct.cpp)
 */
#ifndef __GCT_H__
#define __GCT_H__

#include <gctypes.h>

#define GCT_MAX_CHEATS           64
#define GCT_MAX_CODES_PER_CHEAT  256
#define GCT_NAME_LEN             80

typedef struct {
	char name[GCT_NAME_LEN];
	u32 codes[GCT_MAX_CODES_PER_CHEAT];
	u16 code_count;
	u8 enabled;
} GCT_CheatEntry;

typedef struct {
	char game_id[8];
	char game_title[80];
	GCT_CheatEntry entries[GCT_MAX_CHEATS];
	u16 cheat_count;
} GCT_Cheats;

/* 1 = ok, 0 = missing file, -1 = empty/invalid */
int GCT_OpenTxtFile(const char *path, GCT_Cheats *out);

/* Write selected cheats to path. Returns 1 on success. */
int GCT_CreateGCT(const GCT_Cheats *cheats, const char *path);

int GCT_IsCodeLine(const char *line);

/* If gct_path exists, mark entries whose code blocks appear in the file. */
void GCT_MarkEnabledFromFile(GCT_Cheats *cheats, const char *gct_path);

#endif
