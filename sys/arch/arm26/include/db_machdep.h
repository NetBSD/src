/* $NetBSD: db_machdep.h,v 1.5 2001/03/11 16:31:06 bjh21 Exp $ */

#include <arm/db_machdep.h>

/* arm26 uses ELF */
#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
