#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.1 1993/10/19 08:17:20 deraadt Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a sparc */
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
