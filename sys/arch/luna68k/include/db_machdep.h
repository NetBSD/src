/*	$NetBSD: db_machdep.h,v 1.1.14.1 2002/01/08 00:25:50 nathanw Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
