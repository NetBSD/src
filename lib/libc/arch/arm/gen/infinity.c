/*	$NetBSD: infinity.c,v 1.3 2002/02/19 20:08:19 bjh21 Exp $	*/

/*
 * IEEE-compatible infinity.c -- public domain.
 */

#include <math.h>
#include <machine/endian.h>

const union __double_u __infinity =
#if BYTE_ORDER == BIG_ENDIAN
	{ { 0x7f, 0xf0,    0,    0, 0, 0,    0,    0} };
#else
#ifdef __VFP_FP__
	{ {    0,    0,    0,    0, 0, 0, 0xf0, 0x7f} };
#else
	{ {    0,    0, 0xf0, 0x7f, 0, 0,    0,    0} };
#endif
#endif
