/*	$NetBSD: infinityl.c,v 1.1 2003/10/25 22:31:20 kleink Exp $	*/

/*
 * infinityl.c - max. value representable in VAX D_floating  -- public domain.
 * This is _not_ infinity.
 */

#include <math.h>

const union __long_double_u __infinityl =
	{ { 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
