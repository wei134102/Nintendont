/*
 * Gecko TXT cheat parser and GCT writer (ported from USB Loader GX gct.cpp)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gct.h"
#include "ff_utf8.h"

static const u8 GCT_Header[8] = { 0x00, 0xd0, 0xc0, 0xde, 0x00, 0xd0, 0xc0, 0xde };
static const u8 GCT_Footer[8] = { 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static void RemoveLineEnds(char *str)
{
	char *w = str;
	const char *r = str;
	while (*r) {
		if (*r == '\n' || *r == '\r') {
			r++;
			continue;
		}
		*w++ = *r++;
	}
	*w = 0;
}

static void GCT_TrimLine(char *str)
{
	char *start = str;
	char *end;

	while (*start == ' ' || *start == '\t')
		start++;
	if (start != str)
		memmove(str, start, strlen(start) + 1);

	end = str + strlen(str);
	while (end > str && (end[-1] == ' ' || end[-1] == '\t'))
		end--;
	*end = 0;
}

static int GCT_LineEmpty(const char *line)
{
	return !line || line[0] == 0;
}

int GCT_IsCodeLine(const char *str)
{
	char part1[9];
	char part2[9];

	if (!str || strlen(str) < 17 || str[8] != ' ')
		return 0;

	snprintf(part1, sizeof(part1), "%c%c%c%c%c%c%c%c",
		str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7]);
	snprintf(part2, sizeof(part2), "%c%c%c%c%c%c%c%c",
		str[9], str[10], str[11], str[12], str[13], str[14], str[15], str[16]);

	if (strtok(part1, "0123456789ABCDEFabcdef") == NULL &&
	    strtok(part2, "0123456789ABCDEFabcdef") == NULL)
		return 1;

	return 0;
}

static void GCT_Clear(GCT_Cheats *out)
{
	memset(out, 0, sizeof(*out));
}

static int GCT_CopyLine(const char **cursor, const char *end, char *line, size_t line_len,
	const char **line_start)
{
	size_t i = 0;

	if (*cursor >= end)
		return 0;

	if (line_start)
		*line_start = *cursor;

	while (*cursor < end && **cursor != '\n' && **cursor != '\r' && i + 1 < line_len)
		line[i++] = *(*cursor)++;

	line[i] = 0;
	while (*cursor < end && (**cursor == '\n' || **cursor == '\r'))
		(*cursor)++;

	RemoveLineEnds(line);
	GCT_TrimLine(line);
	return 1;
}

int GCT_OpenTxtFile(const char *path, GCT_Cheats *out)
{
	FIL fp;
	FRESULT fr;
	char *buf = NULL;
	const char *end;
	const char *p;
	char line[512];
	GCT_CheatEntry *entry;
	UINT rd;
	static const u32 txt_max = 512 * 1024;

	GCT_Clear(out);

	fr = f_open_char(&fp, path, FA_READ | FA_OPEN_EXISTING);
	if (fr != FR_OK)
		return 0;

	if (f_size(&fp) == 0 || f_size(&fp) > txt_max) {
		f_close(&fp);
		return -1;
	}

	buf = (char *)malloc((size_t)f_size(&fp) + 1);
	if (!buf) {
		f_close(&fp);
		return -1;
	}

	if (f_read(&fp, buf, (UINT)f_size(&fp), &rd) != FR_OK) {
		free(buf);
		f_close(&fp);
		return -1;
	}
	f_close(&fp);
	buf[rd] = 0;
	end = buf + rd;
	p = buf;

	/* UTF-8 BOM */
	if (rd >= 3 && (u8)buf[0] == 0xEF && (u8)buf[1] == 0xBB && (u8)buf[2] == 0xBF)
		p = buf + 3;

	if (!GCT_CopyLine(&p, end, line, sizeof(line), NULL))
		goto fail;
	strncpy(out->game_id, line, sizeof(out->game_id) - 1);

	if (!GCT_CopyLine(&p, end, line, sizeof(line), NULL))
		goto fail;
	strncpy(out->game_title, line, sizeof(out->game_title) - 1);

	while (p < end && out->cheat_count < GCT_MAX_CHEATS) {
		const char *line_start;

		if (!GCT_CopyLine(&p, end, line, sizeof(line), NULL))
			break;
		if (GCT_LineEmpty(line))
			continue;

		entry = &out->entries[out->cheat_count];
		memset(entry, 0, sizeof(*entry));
		strncpy(entry->name, line, sizeof(entry->name) - 1);

		while (p < end) {
			if (!GCT_CopyLine(&p, end, line, sizeof(line), &line_start))
				break;
			if (GCT_LineEmpty(line))
				break;

			if (GCT_IsCodeLine(line)) {
				u32 a, b;
				if (entry->code_count + 2 > GCT_MAX_CODES_PER_CHEAT)
					continue;
				line[8] = 0;
				line[17] = 0;
				a = (u32)strtoul(&line[0], NULL, 16);
				b = (u32)strtoul(&line[9], NULL, 16);
				entry->codes[entry->code_count++] = a;
				entry->codes[entry->code_count++] = b;
			} else if (entry->code_count > 0) {
				/* 下一条作弊名（TXT 无空行分隔时，与 UGX 列表一致） */
				p = line_start;
				break;
			} else if (entry->comment[0] == 0) {
				strncpy(entry->comment, line, sizeof(entry->comment) - 1);
			}
		}

		if (entry->code_count > 0)
			out->cheat_count++;
	}

	free(buf);
	return (out->cheat_count > 0) ? 1 : -1;

fail:
	free(buf);
	return -1;
}

int GCT_CreateGCT(const GCT_Cheats *cheats, const char *path)
{
	FIL fp;
	UINT bw;
	u16 i;
	FRESULT fr;

	if (!cheats || !path)
		return 0;

	fr = f_open_char(&fp, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (fr != FR_OK)
		return 0;

	if (f_write(&fp, GCT_Header, sizeof(GCT_Header), &bw) != FR_OK || bw != sizeof(GCT_Header)) {
		f_close(&fp);
		return 0;
	}

	for (i = 0; i < cheats->cheat_count; i++) {
		const GCT_CheatEntry *e = &cheats->entries[i];
		UINT bytes;

		if (!e->enabled || e->code_count == 0)
			continue;

		bytes = e->code_count * sizeof(u32);
		if (f_write(&fp, e->codes, bytes, &bw) != FR_OK || bw != bytes) {
			f_close(&fp);
			return 0;
		}
	}

	if (f_write(&fp, GCT_Footer, sizeof(GCT_Footer), &bw) != FR_OK || bw != sizeof(GCT_Footer)) {
		f_close(&fp);
		return 0;
	}

	f_close(&fp);
	return 1;
}

void GCT_MarkEnabledFromFile(GCT_Cheats *cheats, const char *gct_path)
{
	u8 *buf = NULL;
	UINT size = 0;
	FIL fp;
	FRESULT fr;
	u16 i;

	if (!cheats || !gct_path)
		return;

	fr = f_open_char(&fp, gct_path, FA_READ | FA_OPEN_EXISTING);
	if (fr != FR_OK)
		return;

	size = (UINT)f_size(&fp);
	if (size <= sizeof(GCT_Header) + sizeof(GCT_Footer)) {
		f_close(&fp);
		return;
	}

	buf = (u8 *)malloc(size);
	if (!buf) {
		f_close(&fp);
		return;
	}

	if (f_read(&fp, buf, size, &size) != FR_OK) {
		free(buf);
		f_close(&fp);
		return;
	}
	f_close(&fp);

	for (i = 0; i < cheats->cheat_count; i++) {
		GCT_CheatEntry *e = &cheats->entries[i];
		UINT len = e->code_count * sizeof(u32);
		UINT off;

		e->enabled = 0;
		if (len == 0)
			continue;

		/* 整段代码块连续匹配（与 UGX IsCheatIncluded 相同） */
		for (off = sizeof(GCT_Header); off + len <= size - sizeof(GCT_Footer); off += 4) {
			if (memcmp(e->codes, buf + off, len) == 0) {
				e->enabled = 1;
				break;
			}
		}
	}

	free(buf);
}
