/*	$NetBSD: infinityf_ieee754.c,v 1.1 2003/10/25 22:31:20 kleink Exp $	*/

/*
 * IEEE-compatible infinityf.c -- public domain.
 */

#include <math.h>
#include <machine/endian.h>

const union __float_u __infinityf =
#if BYTE_ORDER == BIG_ENDIAN
	{ { 0x7f, 0x80,     0,    0 } };
#else
	{ {    0,    0,  0x80, 0x7f } };
#endif
