/*	$NetBSD: infinity.c,v 1.4 1998/07/26 14:14:15 mycroft Exp $	*/

/*
 * IEEE-compatible infinity.c -- public domain.
 */

#include <math.h>
#include <machine/endian.h>

/* bytes for +Infinity on a MIPS */
#if BYTE_ORDER == BIG_ENDIAN
const char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
#else
const char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
#endif
