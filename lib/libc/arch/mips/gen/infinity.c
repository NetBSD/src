#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.1 1995/01/18 01:27:24 mellon Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a MIPS */
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
