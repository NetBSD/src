/*	$NetBSD: infinityl.c,v 1.1 2003/10/25 22:31:20 kleink Exp $	*/

/*
 * IEEE-compatible infinityl.c for big-endian 128-bit format -- public domain.
 */

#include <math.h>

const union __long_double_u __infinityl =
	{ { 0x7f, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };
