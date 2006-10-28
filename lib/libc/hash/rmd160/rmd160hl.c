/* $NetBSD: rmd160hl.c,v 1.5 2006/10/28 13:05:42 agc Exp $ */

/*
 * Derived from code ritten by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: rmd160hl.c,v 1.5 2006/10/28 13:05:42 agc Exp $");
#endif /* LIBC_SCCS and not lint */

#define	HASH_ALGORITHM	RMD160
#define HASH_INCLUDE	<sys/rmd160.h>

#include "../hashhl.c"
