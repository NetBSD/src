/*	$NetBSD: db_machdep.h,v 1.1.6.2 2007/01/12 01:01:03 ad Exp $	*/

#include <arm/db_machdep.h>

/* zaurus uses ELF for kernel */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE      32
