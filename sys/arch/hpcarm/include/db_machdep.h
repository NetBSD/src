/* $NetBSD: db_machdep.h,v 1.2.4.3 2001/03/27 15:30:51 bouyer Exp $ */

#include <arm/db_machdep.h>

/* hpcarm uses ELF for kernel */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE      32
