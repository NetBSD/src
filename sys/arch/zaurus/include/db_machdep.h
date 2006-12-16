/*	$NetBSD: db_machdep.h,v 1.1 2006/12/16 05:43:53 ober Exp $	*/

#include <arm/db_machdep.h>

/* zaurus uses ELF for kernel */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE      32
