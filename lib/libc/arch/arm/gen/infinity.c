/*	$NetBSD: infinity.c,v 1.5 2003/10/26 16:00:17 kleink Exp $	*/

/*
 * IEEE-compatible infinity.c -- public domain.
 * For VFP (plain double-precision) and
 * for FPA (which is a different layout thereof).
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
