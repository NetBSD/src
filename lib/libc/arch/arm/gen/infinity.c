/*	$NetBSD: infinity.c,v 1.4 2003/10/25 16:17:44 kleink Exp $	*/

/*
 * IEEE-compatible infinity.c -- public domain.
 */

#include <math.h>
#include <machine/endian.h>

const union __double_u __infinity =
#ifdef __VFP_FP__
#if BYTE_ORDER == BIG_ENDIAN
	{ { 0x7f, 0xf0,    0,    0, 0, 0,    0,    0 } };
#else
	{ {    0,    0,    0,    0, 0, 0, 0xf0, 0x7f } };
#endif /* BYTE_ORDER == BIG_ENDIAN */
#else
	{ {    0,    0, 0xf0, 0x7f, 0, 0,    0,    0 } };
#endif /* __VFP_FP__ */
