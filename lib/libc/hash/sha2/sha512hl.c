/* $NetBSD: sha512hl.c,v 1.7 2006/10/28 13:05:42 agc Exp $ */

/*
 * Derived from code ritten by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: sha512hl.c,v 1.7 2006/10/28 13:05:42 agc Exp $");
#endif /* LIBC_SCCS and not lint */

#define	HASH_ALGORITHM	SHA512
#define	HASH_FNPREFIX	SHA512_
#define HASH_INCLUDE	<sys/sha2.h>

#include "../hashhl.c"
