/*	$NetBSD: infinity.c,v 1.4 1998/11/14 19:31:02 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.4 1998/11/14 19:31:02 christos Exp $");
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a PowerPC */
const char __infinity[] = { 0x7f, (char)0xf0, 0, 0, 0, 0, 0, 0 };
