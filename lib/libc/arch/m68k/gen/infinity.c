#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.1 1993/11/25 23:37:05 paulus Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 68k */
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
