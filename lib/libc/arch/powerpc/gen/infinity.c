#ifndef lint
static char rcsid[] = "$NetBSD: infinity.c,v 1.1 1997/03/29 20:55:55 thorpej Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a PowerPC */
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
