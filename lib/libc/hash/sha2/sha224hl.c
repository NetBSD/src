/* $NetBSD */

/*
 * Derived from code written by Jason R. Thorpe <thorpej@NetBSD.org>,
 * May 20, 2009.
 * Public domain.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sha224hl.c,v 1.1.2.2.2.1 2010/04/21 05:28:08 matt Exp $");

#define	HASH_ALGORITHM	SHA224
#define	HASH_FNPREFIX	SHA224_
#define HASH_INCLUDE	<sys/sha2.h>

#include "../hashhl.c"
