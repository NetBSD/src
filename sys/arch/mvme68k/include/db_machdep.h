/*	$NetBSD: db_machdep.h,v 1.1.1.1.34.1 2000/12/08 09:28:35 bouyer Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
