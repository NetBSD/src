/*	$NetBSD: nanf.c,v 1.2 2000/04/18 22:07:17 mark Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: nanf.c,v 1.2 2000/04/18 22:07:17 mark Exp $");
#endif /* not lint */

/* nanf.c */

#include <math.h>

/* bytes for quiet NaN on an arm32 (IEEE single precision) */
const char __nanf[] __attribute__((__aligned__(4))) = { 0, 0, (char)0xc0, 0x7f };
