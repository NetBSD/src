/*	$NetBSD: db_machdep.h,v 1.2 2001/12/24 17:57:19 chs Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
