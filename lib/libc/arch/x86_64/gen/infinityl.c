/*	$NetBSD: infinityl.c,v 1.3.2.1 2011/06/23 14:18:36 cherry Exp $	*/

/*
 * IEEE-compatible infinityl.c for little-endian 80-bit format -- public domain.
 * Note that the representation includes 48 bits of tail padding per amd64 ABI.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinityl.c,v 1.3.2.1 2011/06/23 14:18:36 cherry Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>

const union __long_double_u __infinityl =
	{ { 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f, 0, 0, 0, 0, 0, 0 } };
