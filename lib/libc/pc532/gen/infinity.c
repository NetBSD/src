#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.1.1.1 1993/09/17 18:43:47 phil Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a ns32k */
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
