/*	$NetBSD: nanf.c,v 1.2 2002/01/21 23:54:45 ross Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: nanf.c,v 1.2 2002/01/21 23:54:45 ross Exp $");
#endif /* not lint */

/* nanf.c */

#include <math.h>

/* bytes for quiet NaN on an Alpha (IEEE single precision) */
const char __nanf[] __attribute__((__aligned__(__alignof__(float)))) = {
	0, 0, (char)0xc0, 0x7f
};
