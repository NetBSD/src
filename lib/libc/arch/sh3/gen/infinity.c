/*	$NetBSD: infinity.c,v 1.1 2000/01/05 14:07:33 msaitoh Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.1 2000/01/05 14:07:33 msaitoh Exp $");
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a SH3 */
const char __infinity[] = { 0x7f, (char)0xf0, 0, 0, 0, 0, 0, 0 };
