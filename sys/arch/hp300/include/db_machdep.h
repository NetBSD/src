/*	$NetBSD: db_machdep.h,v 1.3 2001/01/02 04:28:38 simonb Exp $	*/

/* Just use the common m68k definition */
#include <m68k/db_machdep.h>

#undef DB_AOUT_SYMBOLS
#define	DB_ELF_SYMBOLS
#define	DB_ELFSIZE 32
