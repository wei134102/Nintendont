#ifndef __CHEAT_MENU_H__
#define __CHEAT_MENU_H__

#include "menu.h"

/**
 * TXT: {device}:/txtcode/{ID}.txt
 * GCT: {device}:/codes/{ID}.gct (Nintendont default search path)
 * On success enables NIN_CFG_CHEATS (no custom CheatPath).
 */
void CheatMenu_Show(const gameinfo *gi);

#endif
