/*	$NetBSD: infinity.c,v 1.3 1997/07/16 14:38:05 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.3 1997/07/16 14:38:05 christos Exp $");
#endif /* not lint */
/*
 * XXX - THIS IS (probably) COMPLETELY WRONG ON VAX!!!
 */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 387 */
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
