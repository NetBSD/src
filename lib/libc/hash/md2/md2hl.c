/* $NetBSD: md2hl.c,v 1.2 2005/09/26 03:01:41 christos Exp $ */

/*
 * Derived from code ritten by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: md2hl.c,v 1.2 2005/09/26 03:01:41 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#define	HASH_ALGORITHM	MD2
#define HASH_INCLUDE	<md2.h>

#include "../hash.c"
