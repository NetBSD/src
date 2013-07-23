/*	$NetBSD: db_machdep.h,v 1.1.2.2 2013/07/23 21:07:34 riastradh Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
