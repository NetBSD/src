/*	$NetBSD: nanf.c,v 1.1.6.1 2002/01/28 20:49:44 nathanw Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: nanf.c,v 1.1.6.1 2002/01/28 20:49:44 nathanw Exp $");
#endif /* not lint */

/* nanf.c */

#include <math.h>

/* bytes for quiet NaN on an Alpha (IEEE single precision) */
const char __nanf[] __attribute__((__aligned__(__alignof__(float)))) = {
	0, 0, (char)0xc0, 0x7f
};
