/* $NetBSD: db_machdep.h,v 1.3 2001/03/20 18:01:49 toshii Exp $ */

#include <arm/db_machdep.h>

/* hpcarm uses ELF for kernel */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE      32
