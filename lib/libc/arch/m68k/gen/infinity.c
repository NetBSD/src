/*	$NetBSD: infinity.c,v 1.2 1995/12/13 19:36:32 thorpej Exp $	*/

#ifndef lint
static char rcsid[] = "$NetBSD: infinity.c,v 1.2 1995/12/13 19:36:32 thorpej Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 68k */
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
