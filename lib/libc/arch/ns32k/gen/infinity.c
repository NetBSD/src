#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.3 1996/04/04 23:45:18 phil Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a ns32k */
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
