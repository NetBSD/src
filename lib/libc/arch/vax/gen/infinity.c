/*	$NetBSD: infinity.c,v 1.7.2.1 2002/03/08 21:34:55 nathanw Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinity.c,v 1.7.2.1 2002/03/08 21:34:55 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */
/*
 * XXX - This is not correct, but what can we do about it???
 */

/* infinity.c */

#include <math.h>

/* The highest D float on a vax. */
const union __double_u __infinity =
	{ { 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
