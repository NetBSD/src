/*	$NetBSD: db_machdep.h,v 1.1.10.2 2002/06/23 17:35:52 jdolecek Exp $	*/

#ifndef _EBVMIPS_DB_MACHDEP_H_
#define _EBVMIPS_DB_MACHDEP_H_

/* XXX: Common for all evbmips or not? */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE	32

#include <mips/db_machdep.h>

#endif	/* !_EBVMIPS_DB_MACHDEP_H_ */
