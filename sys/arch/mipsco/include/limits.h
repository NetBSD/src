/*	$NetBSD: limits.h,v 1.1 2000/08/12 22:58:25 wdk Exp $	*/

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		60		/* ticks per second */
#endif
