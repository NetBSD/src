/*	$NetBSD: limits.h,v 1.1.2.2 2000/11/20 20:14:05 bouyer Exp $	*/

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		60		/* ticks per second */
#endif
