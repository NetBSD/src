/*	$NetBSD: infinity.c,v 1.3 1998/07/26 14:14:15 mycroft Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.3 1998/07/26 14:14:15 mycroft Exp $");
#endif /* not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 387 */
const char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
