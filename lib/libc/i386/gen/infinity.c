#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.2 1993/08/02 17:49:36 mycroft Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 387 */
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
