#ifndef lint
static char rcsid[] = "$Id: infinity.c,v 1.2 1995/06/28 02:18:21 jonathan Exp $";
#endif /* not lint */

/* infinity.c */

#include <math.h>
#include <machine/endian.h>

/* bytes for +Infinity on a MIPS */
#if BYTE_ORDER == BIG_ENDIAN
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
#else
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
#endif
