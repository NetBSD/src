/*	$NetBSD: db_machdep.h,v 1.1.4.2 2006/12/30 20:47:29 yamt Exp $	*/

#include <arm/db_machdep.h>

/* zaurus uses ELF for kernel */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE      32
