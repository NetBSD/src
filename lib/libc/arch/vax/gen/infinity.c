/*	$NetBSD: infinity.c,v 1.5.2.1 1999/04/19 04:36:28 cjs Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.5.2.1 1999/04/19 04:36:28 cjs Exp $");
#endif /* not lint */
/*
 * XXX - This is not correct, but what can we do about it???
 */

/* infinity.c */

#include <math.h>

/* The highest D float on a vax. */
const char __infinity[] = { (char)0xff, (char)0x7f, (char)0xff, (char)0xff,
	(char)0xff, (char)0xff, (char)0xff, (char)0xff };
