/*	$NetBSD: infinityl.c,v 1.1 2003/10/25 22:31:20 kleink Exp $	*/

/*
 * IEEE-compatible infinityl.c -- public domain.
 * For VFP (where long double == double) and
 * for FPA (where an 80-bit format with padding is used).
 */

#include <math.h>
#include <machine/endian.h>

const union __long_double_u __infinityl =
#ifdef __VFP_FP__
#if BYTE_ORDER == BIG_ENDIAN
	{ { 0x7f, 0xf0, 0, 0, 0, 0,    0,    0             } };
#else
	{ {    0,    0, 0, 0, 0, 0, 0xf0, 0x7f             } };
#endif /* BYTE_ORDER == BIG_ENDIAN */
#else
	{ { 0xff, 0x7f, 0, 0, 0, 0,    0,    0, 0, 0, 0, 0 } };
#endif /* __VFP_FP__ */
