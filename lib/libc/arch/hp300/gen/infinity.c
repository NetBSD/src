#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.1 1993/10/07 00:17:49 cgd Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 68k */
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
