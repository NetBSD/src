#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.2 1995/09/20 22:34:03 phil Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a ns32k */
char __infinity[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x7f };
