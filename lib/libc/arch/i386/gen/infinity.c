/*	$NetBSD: infinity.c,v 1.4 1998/11/14 19:31:01 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.4 1998/11/14 19:31:01 christos Exp $");
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 387 */
const char __infinity[] = { 0, 0, 0, 0, 0, 0, (char)0xf0, 0x7f };
