/* $NetBSD: sha1hl.c,v 1.3 2005/09/28 16:31:45 christos Exp $ */

/*
 * Derived from code ritten by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: sha1hl.c,v 1.3 2005/09/28 16:31:45 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#define	HASH_ALGORITHM	SHA1
#define HASH_INCLUDE	<sha1.h>

#include "../hashhl.c"
