/* $NetBSD: db_machdep.h,v 1.2.2.6 2001/03/12 13:27:29 bouyer Exp $ */

#include <arm/db_machdep.h>

/* arm26 uses ELF */
#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32
