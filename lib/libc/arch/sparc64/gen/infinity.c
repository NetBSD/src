/*	$NetBSD: infinity.c,v 1.1 1998/09/11 04:56:26 eeh Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.1 1998/09/11 04:56:26 eeh Exp $");
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a sparc */
const char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
