/*	$NetBSD: infinity.c,v 1.2 1997/07/13 18:45:41 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.2 1997/07/13 18:45:41 christos Exp $");
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a PowerPC */
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
