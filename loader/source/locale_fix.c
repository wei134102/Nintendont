/*
 * Workaround for devkitPPC/newlib vs prebuilt libogc mismatch:
 * libogc ipc.o may reference __locale_ctype_ptr while older libc.a
 * does not export it. Point the symbol at newlib's _ctype_ table.
 */
#include <ctype.h>

extern const char _ctype_[];

const unsigned char *const *__locale_ctype_ptr =
	(const unsigned char *const *)_ctype_;
