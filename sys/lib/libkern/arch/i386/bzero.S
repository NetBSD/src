/*	$NetBSD: bzero.S,v 1.7 2003/04/15 22:49:50 dsl Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: bzero.S,v 1.7 2003/04/15 22:49:50 dsl Exp $")
#endif

#define	BZERO
#include "memset.S"
