/*	$NetBSD: nanf.c,v 1.1 2001/06/19 00:25:03 fvdl Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: nanf.c,v 1.1 2001/06/19 00:25:03 fvdl Exp $");
#endif /* not lint */

/* nanf.c */

#include <math.h>

/* bytes for quiet NaN on a 387 (IEEE single precision) */
const char __nanf[] = { 0, 0, (char)0xc0, 0x7f };
