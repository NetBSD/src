/*	$NetBSD: infinity.c,v 1.2 1997/07/13 18:45:18 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.2 1997/07/13 18:45:18 christos Exp $");
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 387 */
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
