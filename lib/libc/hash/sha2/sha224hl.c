/* $NetBSD */

/*
 * Derived from code written by Jason R. Thorpe <thorpej@NetBSD.org>,
 * May 20, 2009.
 * Public domain.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sha224hl.c,v 1.1.6.2 2009/07/22 22:31:07 snj Exp $");

#define	HASH_ALGORITHM	SHA224
#define	HASH_FNPREFIX	SHA224_
#define HASH_INCLUDE	<sys/sha2.h>

#include "../hashhl.c"
