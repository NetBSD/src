/*	$NetBSD: db_machdep.h,v 1.1.2.2 2006/12/18 11:42:09 yamt Exp $	*/

#include <arm/db_machdep.h>

/* zaurus uses ELF for kernel */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE      32
