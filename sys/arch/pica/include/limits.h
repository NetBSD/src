/*	$NetBSD: limits.h,v 1.2 1996/03/23 03:42:48 jonathan Exp $	*/

#include <mips/limits.h>

#ifdef _KERNEL
#define CLK_TCK		100		/* ticks per second */
#endif
