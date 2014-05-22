/*	$NetBSD: db_machdep.h,v 1.2.4.2 2014/05/22 11:39:44 yamt Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
