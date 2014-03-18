/*	$NetBSD: db_machdep.h,v 1.2 2014/03/18 18:20:41 riastradh Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
