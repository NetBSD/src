/*	$NetBSD: infinity.c,v 1.2 2000/01/17 16:21:36 kleink Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: infinity.c,v 1.2 2000/01/17 16:21:36 kleink Exp $");
#endif /* not lint */

/* infinity.c */

#include <math.h>
#include <machine/endian.h>

/* bytes for +Infinity on a SH3 */
#if BYTE_ORDER == LITTLE_ENDIAN
const char __infinity[] = { 0, 0, 0, 0, 0, 0, (char)0xf0, 0x7f };
#else
const char __infinity[] = { 0x7f, (char)0xf0, 0, 0, 0, 0, 0, 0 };
#endif
