/* $NetBSD: db_machdep.h,v 1.3.8.2 2001/03/20 18:01:50 toshii Exp $ */

#include <arm/db_machdep.h>

/* hpcarm uses ELF for kernel */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE      32
