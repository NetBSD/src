/* $NetBSD: sha256hl.c,v 1.6 2005/09/28 16:31:45 christos Exp $ */

/*
 * Derived from code ritten by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: sha256hl.c,v 1.6 2005/09/28 16:31:45 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#define	HASH_ALGORITHM	SHA256
#define	HASH_FNPREFIX	SHA256_
#define HASH_INCLUDE	<crypto/sha2.h>

#include "../hashhl.c"
