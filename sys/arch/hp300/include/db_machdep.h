/*	$NetBSD: db_machdep.h,v 1.2.32.1 2001/01/05 17:34:13 bouyer Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#undef DB_AOUT_SYMBOLS
#define	DB_ELF_SYMBOLS
#define	DB_ELFSIZE 32
