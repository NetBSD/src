/*	$NetBSD: db_machdep.h,v 1.2 2000/12/02 13:57:05 scw Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
