/*	$NetBSD: infinity.c,v 1.5 1998/11/14 19:31:02 christos Exp $	*/

/*
 * IEEE-compatible infinity.c -- public domain.
 */

#include <math.h>
#include <machine/endian.h>

/* bytes for +Infinity on a MIPS */
#if BYTE_ORDER == BIG_ENDIAN
const char __infinity[] = { 0x7f, (char)0xf0, 0, 0, 0, 0, 0, 0 };
#else
const char __infinity[] = { 0, 0, 0, 0, 0, 0, (char)0xf0, 0x7f };
#endif
