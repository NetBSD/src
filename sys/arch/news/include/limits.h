/*	$NetBSD: limits.h,v 1.1 1998/02/18 13:48:20 tsubai Exp $	*/

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		60		/* ticks per second */
#endif
