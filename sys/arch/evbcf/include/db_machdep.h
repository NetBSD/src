/*	$NetBSD: db_machdep.h,v 1.2.8.2 2014/08/20 00:02:57 tls Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
